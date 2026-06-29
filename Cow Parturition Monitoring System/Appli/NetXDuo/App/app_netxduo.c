/**
 ****************************************************************************************************
 * @file        app_netxduo.c
 * @brief       NetXDuo TCP client for 991 calving alert.
 *              Static-IP mode: board has fixed BOARD_IP, server runs on PC at
 *              TCP_SERVER_IP_ADDRESS. No DHCP (development env is direct PC
 *              connection, no DHCP server).
 ****************************************************************************************************
 */

#include "app_netxduo.h"
#include "nx_stm32_eth_driver.h"
#include "nx_stm32_phy_driver.h"
#include "main.h"               /* HAL_GPIO_WritePin, GPIOE, GPIO_PIN_10, Error_Handler */
#include <stdio.h>
#include <string.h>

/* === Network configuration (point-to-point with PC) =================
 * PC NIC must be configured with TCP_SERVER_IP_ADDRESS, mask 255.255.255.0,
 * gateway empty. Board uses BOARD_IP statically.
 * ================================================================ */
#define BOARD_IP                IP_ADDRESS(192, 168, 2, 20)
#define BOARD_NETMASK           IP_ADDRESS(255, 255, 255, 0)
#define TCP_SERVER_IP_ADDRESS   IP_ADDRESS(192, 168, 2, 11)
#define TCP_SERVER_PORT         8080
#define LOCAL_PORT              8080

#define PAYLOAD_SIZE            1536
#define PACKET_POOL_SIZE        ((PAYLOAD_SIZE + sizeof(NX_PACKET)) * 10)
#define IP_THREAD_STACK_SIZE    (2 * 1024)
#define IP_THREAD_PRIORITY      10
#define LINK_THREAD_STACK_SIZE  (2 * 1024)
#define LINK_THREAD_PRIORITY    11
#define APP_THREAD_STACK_SIZE   (2 * 1024)
#define APP_THREAD_PRIORITY     10
#define LED_THREAD_STACK_SIZE   (1 * 1024)
#define LED_THREAD_PRIORITY     12

/* LED1 = PE10, 低电平点亮 */
#define LED1_ON()   HAL_GPIO_WritePin(GPIOE, GPIO_PIN_10, GPIO_PIN_RESET)
#define LED1_OFF()  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_10, GPIO_PIN_SET)

/* === Step 5: alert queue ============================================
 * Producers (e.g. pp_thread in app.c) call app_netxduo_post_alert(),
 * which enqueues one alert_msg_t and returns immediately. app_thread
 * dequeues and serializes to JSON over the open TCP socket.
 * ================================================================ */
typedef struct {
    const char *class_name;   /* points into nn_classes_table[] in app.c */
    float       conf;
} alert_msg_t;

#define ALERT_MSG_WORDS     (sizeof(alert_msg_t) / sizeof(ULONG))
#define ALERT_QUEUE_DEPTH   4
static TX_QUEUE alert_queue;
static alert_msg_t alert_queue_storage[ALERT_QUEUE_DEPTH];

typedef enum {
    NET_INIT = 0,           /* LED 常灭：未初始化 / 链路 down */
    NET_LINK_UP_NO_TCP,     /* 每 2s 短亮：链路 up + 静态 IP 就绪，TCP 未连 */
    NET_TCP_CONNECTED,      /* 快闪 0.1/0.1：TCP 已连接到 PC 服务器 */
} net_state_t;

static volatile net_state_t net_state = NET_INIT;

static NX_PACKET_POOL packet_pool;
static NX_IP ip_instance;
static TX_THREAD link_thread;
static TX_THREAD app_thread;
static TX_THREAD led_thread;

static VOID link_thread_entry(ULONG id);
static VOID app_thread_entry(ULONG id);
static VOID led_thread_entry(ULONG id);

UINT app_netxduo_init(VOID *memory_ptr)
{
    TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL*)memory_ptr;
    CHAR *pointer;

    printf("[NET] Board IP : %lu.%lu.%lu.%lu (static)\r\n",
           (BOARD_IP >> 24) & 0xFF, (BOARD_IP >> 16) & 0xFF,
           (BOARD_IP >> 8) & 0xFF, BOARD_IP & 0xFF);
    printf("[NET] TCP Server: %lu.%lu.%lu.%lu:%u\r\n",
           (TCP_SERVER_IP_ADDRESS >> 24) & 0xFF, (TCP_SERVER_IP_ADDRESS >> 16) & 0xFF,
           (TCP_SERVER_IP_ADDRESS >> 8) & 0xFF, TCP_SERVER_IP_ADDRESS & 0xFF,
           TCP_SERVER_PORT);

    nx_system_initialize();

    if (tx_byte_allocate(byte_pool, (VOID **)&pointer, PACKET_POOL_SIZE, TX_NO_WAIT) != TX_SUCCESS)
        return TX_POOL_ERROR;
    if (nx_packet_pool_create(&packet_pool, "Packet Pool", PAYLOAD_SIZE, pointer, PACKET_POOL_SIZE) != NX_SUCCESS)
        return NX_POOL_ERROR;

    if (tx_byte_allocate(byte_pool, (VOID **)&pointer, IP_THREAD_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
        return TX_POOL_ERROR;

    /* Create IP with the static address baked in. No DHCP. */
    if (nx_ip_create(&ip_instance, "IP Instance", BOARD_IP, BOARD_NETMASK,
                     &packet_pool, nx_stm32_eth_driver,
                     pointer, IP_THREAD_STACK_SIZE, IP_THREAD_PRIORITY) != NX_SUCCESS)
        return NX_NOT_SUCCESSFUL;

    if (tx_byte_allocate(byte_pool, (VOID **)&pointer, 1024, TX_NO_WAIT) != TX_SUCCESS)
        return TX_POOL_ERROR;
    if (nx_arp_enable(&ip_instance, (VOID *)pointer, 1024) != NX_SUCCESS)
        return NX_NOT_SUCCESSFUL;
    if (nx_icmp_enable(&ip_instance) != NX_SUCCESS)
        return NX_NOT_SUCCESSFUL;
    if (nx_tcp_enable(&ip_instance) != NX_SUCCESS)
        return NX_NOT_SUCCESSFUL;
    if (nx_udp_enable(&ip_instance) != NX_SUCCESS)
        return NX_NOT_SUCCESSFUL;

    if (tx_byte_allocate(byte_pool, (VOID **)&pointer, LINK_THREAD_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
        return TX_POOL_ERROR;
    if (tx_thread_create(&link_thread, "Link Thread", link_thread_entry, 0, pointer,
                         LINK_THREAD_STACK_SIZE, LINK_THREAD_PRIORITY, LINK_THREAD_PRIORITY,
                         TX_NO_TIME_SLICE, TX_AUTO_START) != TX_SUCCESS)
        return TX_THREAD_ERROR;

    if (tx_byte_allocate(byte_pool, (VOID **)&pointer, APP_THREAD_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
        return TX_POOL_ERROR;
    if (tx_thread_create(&app_thread, "App Thread", app_thread_entry , 0, pointer,
                         APP_THREAD_STACK_SIZE, APP_THREAD_PRIORITY, APP_THREAD_PRIORITY,
                         TX_NO_TIME_SLICE, TX_DONT_START) != TX_SUCCESS)
        return TX_THREAD_ERROR;

    if (tx_byte_allocate(byte_pool, (VOID **)&pointer, LED_THREAD_STACK_SIZE, TX_NO_WAIT) != TX_SUCCESS)
        return TX_POOL_ERROR;
    if (tx_thread_create(&led_thread, "Net LED Thread", led_thread_entry, 0, pointer,
                         LED_THREAD_STACK_SIZE, LED_THREAD_PRIORITY, LED_THREAD_PRIORITY,
                         TX_NO_TIME_SLICE, TX_AUTO_START) != TX_SUCCESS)
        return TX_THREAD_ERROR;

    /* Alert queue: depth 4, each message is sizeof(alert_msg_t)/4 ULONGs. */
    if (tx_queue_create(&alert_queue, "Alert Queue", ALERT_MSG_WORDS,
                        alert_queue_storage,
                        ALERT_QUEUE_DEPTH * sizeof(alert_msg_t)) != TX_SUCCESS)
        return TX_QUEUE_ERROR;

    return TX_SUCCESS;
}

void app_netxduo_post_alert(const char *class_name, float conf)
{
    alert_msg_t msg = { class_name, conf };
    /* TX_NO_WAIT: never block the caller (pp_thread runs the AI pipeline).
     * If the queue is full, drop. The 5-sec global debounce in pp_thread
     * means this rarely happens in practice. */
    (void)tx_queue_send(&alert_queue, &msg, TX_NO_WAIT);
}

static VOID led_thread_entry(ULONG id)
{
    (void)id;
    /* TX_TIMER_TICKS_PER_SECOND=1000 → NX_IP_PERIODIC_RATE=1000 (1 tick = 1ms).
     *   100ms  = NX_IP_PERIODIC_RATE / 10
     *   2000ms = NX_IP_PERIODIC_RATE * 2
     */
    while (1)
    {
        switch (net_state)
        {
        case NET_INIT:
            LED1_OFF();
            tx_thread_sleep(NX_IP_PERIODIC_RATE / 10);
            break;
        case NET_LINK_UP_NO_TCP:
            LED1_ON();  tx_thread_sleep(NX_IP_PERIODIC_RATE / 10);
            LED1_OFF(); tx_thread_sleep(NX_IP_PERIODIC_RATE * 2);
            break;
        case NET_TCP_CONNECTED:
            /* 500ms on / 500ms off = 1Hz, unmissable visual confirmation of
             * "socket is alive". If this ever looks solid-on, it means
             * led_thread is being starved by the IP thread. */
            LED1_ON();  tx_thread_sleep(NX_IP_PERIODIC_RATE / 2);
            LED1_OFF(); tx_thread_sleep(NX_IP_PERIODIC_RATE / 2);
            break;
        default:
            LED1_OFF();
            tx_thread_sleep(NX_IP_PERIODIC_RATE / 10);
            break;
        }
    }
}

/* Send a NUL-terminated string over the TCP socket. Returns NX_SUCCESS or
 * the underlying NetX error. Helper to keep app_thread_entry readable. */
static UINT app_netxduo_send_text(NX_TCP_SOCKET *sock, const char *text)
{
    NX_PACKET *pkt;
    UINT s;

    s = nx_packet_allocate(&packet_pool, &pkt, NX_TCP_PACKET, TX_WAIT_FOREVER);
    if (s != NX_SUCCESS) return s;

    s = nx_packet_data_append(pkt, (VOID *)text, strlen(text),
                              &packet_pool, TX_WAIT_FOREVER);
    if (s != NX_SUCCESS) { nx_packet_release(pkt); return s; }

    s = nx_tcp_socket_send(sock, pkt, 10 * NX_IP_PERIODIC_RATE);
    if (s != NX_SUCCESS) nx_packet_release(pkt);
    return s;
}

static VOID app_thread_entry(ULONG id)
{
    (void)id;
    NX_TCP_SOCKET tcp_client_socket;
    UINT ret;
    alert_msg_t alert;
    char json_buf[160];
    int json_len;

    /* Link is already up by the time link_thread resumes us. With static IP,
     * we already have an address, so jump straight to TCP. */

    if (nx_tcp_socket_create(&ip_instance, &tcp_client_socket, "TCP Client Socket",
                             NX_IP_NORMAL, NX_FRAGMENT_OKAY, NX_IP_TIME_TO_LIVE,
                             512, NX_NULL, NX_NULL) != NX_SUCCESS)
        Error_Handler();

    if (nx_tcp_client_socket_bind(&tcp_client_socket, LOCAL_PORT, NX_WAIT_FOREVER) != NX_SUCCESS)
        Error_Handler();

    printf("[NET] Connecting to TCP server");
    while (1)
    {
        ret = nx_tcp_client_socket_connect(&tcp_client_socket, TCP_SERVER_IP_ADDRESS,
                                           TCP_SERVER_PORT, NX_IP_PERIODIC_RATE);
        if (ret == NX_SUCCESS) break;
        if (ret == NX_NOT_CONNECTED) { printf("."); continue; }
        printf("\r\n[NET] connect error 0x%lx\r\n", (unsigned long)ret);
        Error_Handler();
    }
    net_state = NET_TCP_CONNECTED;
    printf("\r\n[NET] Connected.\r\n");

    /* Initial HELLO so the server log shows the connection is alive. */
    ret = app_netxduo_send_text(&tcp_client_socket, "HELLO from STM32N6\r\n");
    if (ret != NX_SUCCESS)
    {
        printf("[NET] HELLO send failed (0x%lx) — connection may be stale "
               "(did you restart the server while board was connected?). "
               "Reset the board.\r\n", (unsigned long)ret);
    }
    else
    {
        printf("[NET] HELLO sent.\r\n");
    }

    /* Main loop: wait for an alert message from pp_thread, format it as
     * JSON, ship it. */
    while (1)
    {
        if (tx_queue_receive(&alert_queue, &alert, TX_WAIT_FOREVER) != TX_SUCCESS)
            continue;

        json_len = snprintf(json_buf, sizeof(json_buf),
                            "{\"event\":\"detection\",\"class\":\"%s\",\"conf\":%.2f}\r\n",
                            alert.class_name ? alert.class_name : "unknown",
                            (double)alert.conf);
        if (json_len <= 0 || json_len >= (int)sizeof(json_buf)) continue;

        ret = app_netxduo_send_text(&tcp_client_socket, json_buf);
        if (ret == NX_SUCCESS)
        {
            printf("[NET] Sent alert: %s (conf=%.2f)\r\n",
                   alert.class_name ? alert.class_name : "unknown",
                   (double)alert.conf);
        }
        else
        {
            printf("[NET] Send failed (0x%lx); will keep socket open and retry next alert\r\n",
                   (unsigned long)ret);
        }
    }
}

static VOID link_thread_entry(ULONG id)
{
    (void)id;
    UINT status;
    ULONG actual_status;
    UINT linkdown = 2;
    int32_t link_state;

    while (1)
    {
        status = nx_ip_interface_status_check(&ip_instance, 0, NX_IP_LINK_ENABLED,
                                              &actual_status, 10);
        if (status == NX_SUCCESS)
        {
            if ((linkdown == 1) || (linkdown == 2))
            {
                linkdown = 0;
                printf("[NET] Link up.\r\n");

                link_state = nx_eth_phy_get_link_state();
                if (link_state == ETH_PHY_STATUS_100MBITS_FULLDUPLEX) printf("[NET] 100Mbps full-duplex.\r\n");
                else if (link_state == ETH_PHY_STATUS_100MBITS_HALFDUPLEX) printf("[NET] 100Mbps half-duplex.\r\n");
                else if (link_state == ETH_PHY_STATUS_10MBITS_FULLDUPLEX) printf("[NET] 10Mbps full-duplex.\r\n");
                else printf("[NET] 10Mbps half-duplex.\r\n");

                nx_ip_driver_direct_command(&ip_instance, NX_LINK_ENABLE, &actual_status);

                if (net_state != NET_TCP_CONNECTED) net_state = NET_LINK_UP_NO_TCP;

                tx_thread_resume(&app_thread);
                tx_thread_relinquish();
            }
        }
        else
        {
            if ((linkdown == 0) || (linkdown == 2))
            {
                linkdown = 1;
                printf("[NET] Link down.\r\n");
                nx_ip_driver_direct_command(&ip_instance, NX_LINK_DISABLE, &actual_status);
                net_state = NET_INIT;
            }
        }

        tx_thread_sleep(NX_IP_PERIODIC_RATE);
    }
}
