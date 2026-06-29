#include "app.h"
#include "app_config.h"
#include "app_utils.h"
#include "app_lcd.h"
#include "app_camera.h"
#include "app_bqueue.h"
#include "app_cpuload.h"
#include "app_postprocess.h"
#include "tx_api.h"
#include "cmw_camera.h"
#include "ll_aton_runtime.h"

/* From Appli/NetXDuo/App/app_netxduo.h — forward-declared to avoid pulling
 * the NetX include path into this file. */
extern void app_netxduo_post_alert(const char *class_name, float conf);

/* 画框与 TCP 报警共用同一个置信度阈值：低于这个值的检测既不显示
 * 绿色矩形框，也不会触发报警通知。改一次两处同步。 */
#define ALERT_CONF_THRESHOLD  0.55f

typedef struct {
    int32_t nb_detect;
    od_pp_outBuffer_t detects[AI_OBJDETECT_YOLOV2_PP_MAX_BOXES_LIMIT];
    uint32_t nn_period_ms;
    uint32_t inf_ms;
    uint32_t pp_ms;
    uint32_t disp_ms;
} app_display_info_t;

typedef struct {
    TX_SEMAPHORE update;
    TX_MUTEX lock;
    app_display_info_t info;
} app_display_t;

static TX_SEMAPHORE isp_semaphore;

static void app_camera_display_pipe_vsync_cb(void);
static void app_camera_display_pipe_frame_cb(void);
static void app_camera_nn_pipe_frame_cb(void);

static TX_THREAD nn_thread;
static UCHAR nn_thread_stack[4096];
static TX_THREAD pp_thread;
static UCHAR pp_thread_stack[4096];
static TX_THREAD dp_thread;
static UCHAR dp_thread_stack[4096];
static TX_THREAD isp_thread;
static UCHAR isp_thread_stack[4096];

static VOID nn_thread_entry(ULONG id);
static VOID pp_thread_entry(ULONG id);
static VOID dp_thread_entry(ULONG id);
static VOID isp_thread_entry(ULONG id);

static app_display_t display;

LL_ATON_DECLARE_NAMED_NN_INSTANCE_AND_INTERFACE(Default);
static uint8_t nn_input_buffers[2][NN_WIDTH * NN_HEIGHT * NN_BPP] __attribute__((aligned(32))) __attribute__((section(".EXTRAM")));
static app_bqueue_t nn_input_queue;
static uint8_t nn_output_buffers[2][ALIGN_VALUE(NN_BUFFER_OUT_SIZE, 32)] __attribute__((aligned(32)));
static app_bqueue_t nn_output_queue;
static const char *nn_classes_table[NN_CLASSES] = NN_CLASSES_TABLE;

static app_cpuload_t cpuload;

static void app_display_static_overlay(void);
static void app_display_network_output(app_display_info_t *display_info);

/* ===== 临时 LED 启动诊断（LED0 = GPIOG PIN10, 低电平点亮）===== */
/* diag_led_stage(N): 闪烁 N 次 = 启动流程到达阶段 N。            */
/* 看 LED 停在第几次闪烁，就知道卡在哪一步。验证完整段删除即可。  */
#define DIAG_BLINK_ITERS    60000000UL
#define DIAG_GAP_ITERS     250000000UL
static void diag_led_stage(uint32_t stage)
{
    volatile uint32_t i;
    uint32_t k;

    HAL_GPIO_WritePin(GPIOG, GPIO_PIN_10, GPIO_PIN_SET);   /* LED0 灭 */
    for (i = 0; i < DIAG_GAP_ITERS; i++) { __NOP(); }      /* 阶段间长间隔 */

    for (k = 0; k < stage; k++)
    {
        HAL_GPIO_WritePin(GPIOG, GPIO_PIN_10, GPIO_PIN_RESET); /* 亮 */
        for (i = 0; i < DIAG_BLINK_ITERS; i++) { __NOP(); }
        HAL_GPIO_WritePin(GPIOG, GPIO_PIN_10, GPIO_PIN_SET);   /* 灭 */
        for (i = 0; i < DIAG_BLINK_ITERS; i++) { __NOP(); }
    }
}
/* ===== 诊断代码结束 ===== */

void app_run(void)
{
    diag_led_stage(1);   /* 阶段1: 进入 app_run */
    app_lcd_init();
    diag_led_stage(2);   /* 阶段2: LCD 初始化完成 */
    app_bqueue_init(&nn_input_queue, 2, (uint8_t *[2]){nn_input_buffers[0], nn_input_buffers[1]});
    app_bqueue_init(&nn_output_queue, 2, (uint8_t *[2]){nn_output_buffers[0], nn_output_buffers[1]});
    app_cpuload_init(&cpuload);
    app_camera_init(app_camera_display_pipe_vsync_cb, app_camera_display_pipe_frame_cb, NULL, app_camera_nn_pipe_frame_cb);
    diag_led_stage(3);   /* 阶段3: 摄像头初始化完成 */

    tx_semaphore_create(&isp_semaphore, NULL, 0);
    tx_semaphore_create(&display.update, NULL, 0);
    tx_mutex_create(&display.lock, NULL, TX_INHERIT);

    app_camera_display_pipe_start(app_lcd_get_bg_buffer(), CMW_MODE_CONTINUOUS);
    app_display_static_overlay();
    diag_led_stage(4);   /* 阶段4: 摄像头→显示管线已启动 */

    tx_thread_create(&nn_thread, "NN Thread", nn_thread_entry, 0, nn_thread_stack, sizeof(nn_thread_stack), TX_MAX_PRIORITIES - 3, TX_MAX_PRIORITIES - 3, 10, TX_DONT_START);
    tx_thread_create(&pp_thread, "PP Thread", pp_thread_entry, 0, pp_thread_stack, sizeof(pp_thread_stack), TX_MAX_PRIORITIES - 2, TX_MAX_PRIORITIES - 2, 10, TX_AUTO_START);
    tx_thread_create(&dp_thread, "DP Thread", dp_thread_entry, 0, dp_thread_stack, sizeof(dp_thread_stack), TX_MAX_PRIORITIES - 2, TX_MAX_PRIORITIES - 2, 10, TX_AUTO_START);
    tx_thread_create(&isp_thread, "ISP Thread", isp_thread_entry, 0, isp_thread_stack, sizeof(isp_thread_stack), TX_MAX_PRIORITIES - 4, TX_MAX_PRIORITIES - 4, 10, TX_AUTO_START);
    diag_led_stage(5);   /* 阶段5: 4 个线程已创建，app_run 正常返回 */
    tx_thread_resume(&nn_thread);
}

static void app_camera_display_pipe_vsync_cb(void)
{
    tx_semaphore_put(&isp_semaphore);
}

static void app_camera_display_pipe_frame_cb(void)
{
    app_lcd_switch_bg_buffer();
    app_camera_display_pipe_set_address(app_lcd_get_bg_buffer());
}

static void app_camera_nn_pipe_frame_cb(void)
{
    uint8_t *buffer;

    buffer = app_bqueue_get_free(&nn_input_queue, 0);
    if (buffer != NULL)
    {
        app_camera_nn_pipe_set_address(buffer);
        app_bqueue_put_ready(&nn_input_queue);
    }
}

static VOID nn_thread_entry(ULONG id)
{
    uint32_t nn_out_len;
    uint32_t nn_in_len;
    uint8_t *nn_pipe_dst;
    uint8_t *capture_buffer;
    uint8_t *output_buffer;
    uint32_t nn_period[2];
    uint32_t nn_period_ms;
    uint32_t time_stamp;
    uint32_t inf_ms;

    nn_in_len = LL_Buffer_len(LL_ATON_Input_Buffers_Info_Default());
    nn_out_len = LL_Buffer_len(LL_ATON_Output_Buffers_Info_Default());

    nn_period[1] = HAL_GetTick();

    nn_pipe_dst = app_bqueue_get_free(&nn_input_queue, 0);

    app_camera_nn_pipe_start(nn_pipe_dst, CMW_MODE_CONTINUOUS);

    while (1)
    {
        nn_period[0] = nn_period[1];
        nn_period[1] = HAL_GetTick();
        nn_period_ms = nn_period[1] - nn_period[0];

        capture_buffer = app_bqueue_get_ready(&nn_input_queue);
        output_buffer = app_bqueue_get_free(&nn_output_queue, 1);

        time_stamp = HAL_GetTick();
        /* uint8 RGB888 (0~255) -> int8 (-128~+127): match model input quant (scale=1/127.5, offset=0) */
        for (uint32_t i = 0; i < nn_in_len; i++) {
            ((int8_t *)capture_buffer)[i] = (int8_t)((int32_t)capture_buffer[i] - 128);
        }
        /* buffer lives in .EXTRAM; clean D-cache so NPU/DMA sees CPU-modified bytes */
        SCB_CleanDCache_by_Addr((uint32_t *)capture_buffer, nn_in_len);
        LL_ATON_Set_User_Input_Buffer_Default(0, capture_buffer, nn_in_len);
        SCB_InvalidateDCache_by_Addr(output_buffer, nn_out_len);
        LL_ATON_Set_User_Output_Buffer_Default(0, output_buffer, nn_out_len);
        LL_ATON_RT_Main(&NN_Instance_Default);
        inf_ms = HAL_GetTick() - time_stamp;

        app_bqueue_put_free(&nn_input_queue);
        app_bqueue_put_ready(&nn_output_queue);

        tx_mutex_get(&display.lock, TX_WAIT_FOREVER);
        display.info.inf_ms = inf_ms;
        display.info.nn_period_ms = nn_period_ms;
        tx_mutex_put(&display.lock);
    }
}

static VOID pp_thread_entry(ULONG id)
{
    yolov2_pp_static_param_t pp_params;
    uint8_t *output_buffer;
    od_pp_out_t pp_output;
    uint32_t nn_pp[2];
    int32_t pp_error;
    int32_t nb_detect;
    int32_t i;
    /* Throttle: only refresh the displayed detection every PP_DISPLAY_PERIOD inferences */
    #define PP_DISPLAY_PERIOD 5
    static uint32_t pp_frame_counter = 0;
    static od_pp_outBuffer_t pp_cached_detect;
    static int32_t pp_cached_nb = 0;
    /* Warmup: suppress detection display for the first PP_WARMUP_MS after startup */
    #define PP_WARMUP_MS 10000
    static uint32_t pp_start_tick = 0;

    app_postprocess_init(&pp_params);

    while (1)
    {
        output_buffer = app_bqueue_get_ready(&nn_output_queue);
        pp_output.nb_detect = 0;
        pp_output.pOutBuff = NULL;

        nn_pp[0] = HAL_GetTick();
        pp_error = app_postprocess_run((void *[]){(void *)output_buffer}, 1, &pp_output, &pp_params);
        nn_pp[1] = HAL_GetTick();

        if (pp_start_tick == 0) pp_start_tick = HAL_GetTick();
        int warmup_active = ((HAL_GetTick() - pp_start_tick) < PP_WARMUP_MS);

        tx_mutex_get(&display.lock, TX_WAIT_FOREVER);
        if (warmup_active || (pp_error != 0) || (pp_output.pOutBuff == NULL) || (pp_output.nb_detect < 0))
        {
            display.info.nb_detect = 0;
            pp_cached_nb = 0;
        }
        else
        {
            nb_detect = pp_output.nb_detect;
            pp_frame_counter++;

            /* Refresh cached single-best detection only every PP_DISPLAY_PERIOD inferences */
            if ((pp_frame_counter % PP_DISPLAY_PERIOD) == 0)
            {
                if (nb_detect > 0)
                {
                    int32_t max_idx = 0;
                    for (i = 1; i < nb_detect; i++)
                    {
                        if (pp_output.pOutBuff[i].conf > pp_output.pOutBuff[max_idx].conf)
                        {
                            max_idx = i;
                        }
                    }
                    pp_cached_detect = pp_output.pOutBuff[max_idx];
                    pp_cached_nb = 1;
                }
                else
                {
                    pp_cached_nb = 0;
                }
            }

            display.info.nb_detect = pp_cached_nb;
            if (pp_cached_nb > 0)
            {
                display.info.detects[0] = pp_cached_detect;
            }
        }
        display.info.pp_ms = nn_pp[1] - nn_pp[0];
        tx_mutex_put(&display.lock);

        /* === Step 5: 检出分娩特征 → TCP 报警 ===
         * sac / hoof / psac 三类同等紧急，任一被检出且 conf > 阈值就通知。
         * 全局去抖：所有类别共用一个 5 秒禁言期，避免持续观察一头牛时被
         * 同一事件刷屏。pp_cached_detect 每 5 帧更新一次（display 节流
         * 的副产物），实际去抖瓶颈是这里的 5 秒，不是缓存。 */
        #define ALERT_DEBOUNCE_MS     5000U
        static uint32_t last_alert_tick = 0;

        if (pp_cached_nb > 0
            && pp_cached_detect.class_index >= 0
            && pp_cached_detect.class_index < NN_CLASSES
            && pp_cached_detect.conf > ALERT_CONF_THRESHOLD)
        {
            uint32_t now = HAL_GetTick();
            if (now - last_alert_tick > ALERT_DEBOUNCE_MS)
            {
                printf("[ALERT] %s conf=%.2f -> posting TCP notification\r\n",
                       nn_classes_table[pp_cached_detect.class_index],
                       (double)pp_cached_detect.conf);
                app_netxduo_post_alert(nn_classes_table[pp_cached_detect.class_index],
                                       pp_cached_detect.conf);
                last_alert_tick = now;
            }
        }

        app_bqueue_put_free(&nn_output_queue);
        tx_semaphore_ceiling_put(&display.update, 1);
    }
}

static VOID dp_thread_entry(ULONG id)
{
    uint32_t disp_ms = 0;
    app_display_info_t display_info;
    uint32_t time_stamp;

    while (1)
    {
        tx_semaphore_get(&display.update, TX_WAIT_FOREVER);
        tx_mutex_get(&display.lock, TX_WAIT_FOREVER);
        display_info = display.info;
        tx_mutex_put(&display.lock);
        display_info.disp_ms = disp_ms;

        time_stamp = HAL_GetTick();
        app_display_network_output(&display_info);
        disp_ms = HAL_GetTick() - time_stamp;
    }
}

static VOID isp_thread_entry(ULONG id)
{
    while (1)
    {
        tx_semaphore_get(&isp_semaphore, TX_WAIT_FOREVER);

        app_camera_isp_update();
    }
}

static uint8_t app_clamp_point(int32_t *x, int32_t *y)
{
    int32_t xi;
    int32_t yi;

    xi = *x;
    yi = *y;

    if (*x < 0)
    {
        *x = 0;
    }

    if (*y < 0)
    {
        *y = 0;
    }

    if (*x >= LCD_BG_WIDTH)
    {
        *x = LCD_BG_WIDTH - 1;
    }

    if (*y >= LCD_BG_HEIGHT)
    {
        *y = LCD_BG_HEIGHT - 1;
    }

    return (xi != *x) || (yi != *y);
}

static void app_display_detection(od_pp_outBuffer_t *detect)
{
    int32_t xc;
    int32_t yc;
    int32_t x0;
    int32_t y0;
    int32_t x1;
    int32_t y1;
    int32_t w;
    int32_t h;

    if ((detect->class_index < 0) || (detect->class_index >= NN_CLASSES))
    {
        return;
    }

    /* 置信度不够就不画框：避免低分检测刷屏，与 TCP 报警共用阈值。 */
    if (detect->conf <= ALERT_CONF_THRESHOLD)
    {
        return;
    }

    xc = (int32_t)(LCD_BG_WIDTH * detect->x_center);
    yc = (int32_t)(LCD_BG_HEIGHT * detect->y_center);
    w = (int32_t)(LCD_BG_WIDTH * detect->width);
    h = (int32_t)(LCD_BG_HEIGHT * detect->height);

    x0 = xc - (w + 1) / 2;
    y0 = yc - (h + 1) / 2;
    x1 = xc + (w + 1) / 2;
    y1 = yc + (h + 1) / 2;

    app_clamp_point(&x0, &y0);
    app_clamp_point(&x1, &y1);

    if ((x1 <= x0) || (y1 <= y0))
    {
        return;
    }

    UTIL_LCD_DrawRect(x0, y0, x1 - x0, y1 - y0, UTIL_LCD_COLOR_GREEN);
    /* 不再在标注框旁边画类别名字 —— 养殖员看屏幕只关心"有没有"，
     * 类别细节可以从串口/PC 服务器报警里看。 */
}

static void app_display_static_overlay(void)
{
    app_display_info_t display_info = {0};

    app_lcd_draw_area_update();
    UTIL_LCD_FillRect(0, 0, LCD_FG_WIDTH, LCD_FG_HEIGHT, 0x00000000);
    UTIL_LCDEx_PrintfAt(0, LINE(0), CENTER_MODE, "Livestock Monitor");
    UTIL_LCDEx_PrintfAt(0, LINE(2), CENTER_MODE, "System Ready");
    UTIL_LCDEx_PrintfAt(0, LINE(4), CENTER_MODE, "Targets");
    UTIL_LCDEx_PrintfAt(0, LINE(5), CENTER_MODE, "0");
    app_lcd_draw_area_commit();

    tx_mutex_get(&display.lock, TX_WAIT_FOREVER);
    display.info = display_info;
    tx_mutex_put(&display.lock);
}

static void app_display_network_output(app_display_info_t *display_info)
{
    float cpuload_one_second;
    uint8_t line_nb = 0;
    int32_t i;

    /* Sticky 显示：高分检测出现后，至少在屏幕上保留 DISPLAY_HOLD_MS，
     * 避免帧间检出不稳定导致绿框闪烁；新合格检测会立刻覆盖旧 sticky。 */
    #define DISPLAY_HOLD_MS 4000U
    static od_pp_outBuffer_t sticky_detect;
    static uint32_t sticky_tick = 0;
    static int sticky_valid = 0;
    int drew_qualifying = 0;
    uint8_t targets_line;

    app_lcd_draw_area_update();

    UTIL_LCD_FillRect(0, 0, LCD_FG_WIDTH, LCD_FG_HEIGHT, 0x00000000);

    app_cpuload_update(&cpuload);
    app_cpuload_get_info(&cpuload, NULL, &cpuload_one_second, NULL);

    UTIL_LCDEx_PrintfAt(0, LINE(line_nb), RIGHT_MODE, "CPU load");
    line_nb += 1;
    UTIL_LCDEx_PrintfAt(0, LINE(line_nb), RIGHT_MODE, "%.1f%%", cpuload_one_second);
    line_nb += 2;
    UTIL_LCDEx_PrintfAt(0, LINE(line_nb), RIGHT_MODE, "Inference");
    line_nb += 1;
    UTIL_LCDEx_PrintfAt(0, LINE(line_nb), RIGHT_MODE, "%ums", display_info->inf_ms);
    line_nb += 2;
    UTIL_LCDEx_PrintfAt(0, LINE(line_nb), RIGHT_MODE, "FPS");
    line_nb += 1;
    UTIL_LCDEx_PrintfAt(0, LINE(line_nb), RIGHT_MODE, "%.2f", 1000.0 / display_info->nn_period_ms);
    line_nb += 2;
    UTIL_LCDEx_PrintfAt(0, LINE(line_nb), RIGHT_MODE, "Targets");
    line_nb += 1;
    /* Targets 数延后到主循环结束后再画：本帧有合格检测就显示 1，否则 0。
     * 不做累加（用户口径：要么有、要么没有，不关心数量）。 */
    targets_line = line_nb;

    for (i = 0; i < display_info->nb_detect; i++)
    {
        od_pp_outBuffer_t *d = &display_info->detects[i];
        if (d->conf > ALERT_CONF_THRESHOLD
            && d->class_index >= 0 && d->class_index < NN_CLASSES)
        {
            sticky_detect = *d;
            sticky_tick = HAL_GetTick();
            sticky_valid = 1;
            drew_qualifying = 1;
        }
        app_display_detection(d);
    }
    UTIL_LCDEx_PrintfAt(0, LINE(targets_line), RIGHT_MODE, "%u", (unsigned)drew_qualifying);
    if (!drew_qualifying && sticky_valid
        && (HAL_GetTick() - sticky_tick) < DISPLAY_HOLD_MS)
    {
        app_display_detection(&sticky_detect);
    }

    app_lcd_draw_area_commit();
}
