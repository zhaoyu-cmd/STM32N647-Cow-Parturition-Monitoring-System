/**
 ****************************************************************************************************
 * @file        rs485.h
 * @version     V1.0
 * @date        2025-01-13
 * @brief       RS485驱动代码
 ****************************************************************************************************
 * @attention
 * 
 * 
 ****************************************************************************************************
 */

#ifndef __RS485_H
#define __RS485_H

#include "main.h"

/* 引脚定义 */
#define RS485_RE_GPIO_PORT  GPIOG
#define RS485_RE_GPIO_PIN   GPIO_PIN_10

/* IO操作 */
#define RS485_RE(x)         do { (x) ?                                                                      \
                                HAL_GPIO_WritePin(RS485_RE_GPIO_PORT, RS485_RE_GPIO_PIN, GPIO_PIN_SET):     \
                                HAL_GPIO_WritePin(RS485_RE_GPIO_PORT, RS485_RE_GPIO_PIN, GPIO_PIN_RESET);   \
                            } while (0)

/* RS485定义 */
#define RS485_RX_BUF_SIZE   64

/* 函数声明 */
void rs485_init(uint32_t baudrate);                     /* 初始化RS485 */
void rs485_recv_handler(void);                          /* RS485中断接收处理函数 */
void rs485_send_data(uint8_t *buffer, uint8_t size);    /* RS485发送数据 */
void rs485_recv_data(uint8_t *buffer, uint8_t *size);   /* RS485接收数据 */

#endif
