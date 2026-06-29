/**
 ****************************************************************************************************
 * @file        fdcan.h
 * @version     V1.0
 * @date        2025-01-13
 * @brief       FDCAN驱动代码
 ****************************************************************************************************
 * @attention
 * 
 * 
 ****************************************************************************************************
 */

#ifndef __FDCAN_H
#define __FDCAN_H

#include "main.h"

/* FDCAN定义 */
#define FDCAN_DATA_LENGTH 8

/* 函数声明 */
uint8_t fdcan_init(void);                                   /* 初始化FDCAN */
uint8_t fdcan_send_data(uint8_t *buffer, uint32_t timeout); /* FDCAN发送数据 */
uint8_t fdcan_recv_data(uint8_t *buffer, uint32_t timeout); /* FDCAN接收数据 */

#endif
