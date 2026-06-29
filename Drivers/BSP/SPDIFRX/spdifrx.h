/**
 ****************************************************************************************************
 * @file        spdifrx.h
 * @version     V1.0
 * @date        2025-01-13
 * @brief       SPDIFRX驱动代码
 ****************************************************************************************************
 * @attention
 * 
 * 
 ****************************************************************************************************
 */

#ifndef __SPDIFRX_H
#define __SPDIFRX_H

#include "main.h"

/* 函数声明 */
void spdifrx_init(void);
void spdifrx_sync(void);
void spdifrx_stop_sync(void);
uint8_t spdifrx_sync_is_done(void);
void spdifrx_rx_dma_cplt_user_cb_register(void (*cb)(void));    /* SPDIFRX 注册DMA接收完成回调 */
uint8_t spdifrx_rx_dma(uint8_t *buffer, uint32_t data_amount);  /* SPDIFRX DMA接收 */
void spdifrx_rx_dma_cplt_handler(void);                         /* SPDIFRX DMA接收完成处理函数 */
uint8_t spdifrx_rx_dma_is_busy(void);                           /* SPDIFRX DMA接收是否忙 */
uint32_t spdifrx_get_sample_rate(void);                         /* 获取SPDIFRX采样率 */
void spdifrx_irq_handler(void);                                 /* SPDIFRX中断处理函数 */

#endif
