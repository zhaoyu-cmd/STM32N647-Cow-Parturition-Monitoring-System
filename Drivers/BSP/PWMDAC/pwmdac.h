/**
 ****************************************************************************************************
 * @file        pwmdac.h
 * @version     V1.0
 * @date        2025-01-13
 * @brief       PWMDAC驱动代码
 ****************************************************************************************************
 * @attention
 * 
 * 
 ****************************************************************************************************
 */

#ifndef __PWMDAC_H
#define __PWMDAC_H

#include "main.h"

/* PWMDAC定义 */
#define PWMDAC_TIMX_CH1 TIM_CHANNEL_4

/* 函数声明 */
void pwmdac_init(uint16_t arr, uint16_t psc);   /* 初始化PWMDAC */
void pwmdac_set_voltage(uint16_t voltage);      /* 设置PWMDAC输出电压 */

#endif
