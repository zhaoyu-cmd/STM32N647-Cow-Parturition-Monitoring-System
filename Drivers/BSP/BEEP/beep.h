/**
 ****************************************************************************************************
 * @file        beep.h
 * @version     V1.0
 * @date        2025-01-13
 * @brief       蜂鸣器驱动代码
 ****************************************************************************************************
 * @attention
 * 
 * 
 ****************************************************************************************************
 */

#ifndef __BEEP_H
#define __BEEP_H

#include "main.h"

/* 引脚定义 */
#define BEEP_GPIO_PORT  GPIOD
#define BEEP_GPIO_PIN   GPIO_PIN_3

/* IO操作 */
#define BEEP(x)         do { (x) ?                                                              \
                            HAL_GPIO_WritePin(BEEP_GPIO_PORT, BEEP_GPIO_PIN, GPIO_PIN_SET):     \
                            HAL_GPIO_WritePin(BEEP_GPIO_PORT, BEEP_GPIO_PIN, GPIO_PIN_RESET);   \
                        } while (0)

/* 函数声明 */
void beep_init(void);   /* 初始化蜂鸣器 */

#endif
