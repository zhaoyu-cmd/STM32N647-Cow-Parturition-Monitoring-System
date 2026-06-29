/**
 ****************************************************************************************************
 * @file        eeprom.h
 * @version     V1.0
 * @date        2025-01-13
 * @brief       EEPROM驱动代码
 ****************************************************************************************************
 * @attention
 * 
 * 
 ****************************************************************************************************
 */

#ifndef __EEPROM_H
#define __EEPROM_H

#include "main.h"

/* EEPROM定义 */
#define EEPROM_ADDR (0x50 << 1)

/* 函数声明 */
void eeprom_init(void);                                         /* 初始化EEPROM */
uint8_t eeprom_check(void);                                     /* 检查EEPROM */
void eeprom_read(uint8_t addr, uint8_t *buf, uint16_t size);    /* 读EEPROM */
void eeprom_write(uint8_t addr, uint8_t *buf, uint16_t size);   /* 写EEPROM */

#endif
