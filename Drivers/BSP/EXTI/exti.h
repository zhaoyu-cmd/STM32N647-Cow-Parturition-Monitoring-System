/**
 ****************************************************************************************************
 * @file        exti.h
 * @version     V1.0
 * @date        2025-01-13
 * @brief       外部中断驱动代码
 ****************************************************************************************************
 * @attention
 * 
 * 
 ****************************************************************************************************
 */

#ifndef __EXTI_H
#define __EXTI_H

#include "main.h"

/* 引脚定义 */
#define WKUP_INT_GPIO_PIN   GPIO_PIN_13
#define KEY0_INT_GPIO_PIN   GPIO_PIN_6
#define KEY1_INT_GPIO_PIN   GPIO_PIN_1
#define KEY2_INT_GPIO_PIN   GPIO_PIN_11

/* 函数声明 */
void exti_init(void);   /* 初始化外部中断 */

#endif
