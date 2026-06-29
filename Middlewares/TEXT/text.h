/**
 ****************************************************************************************************
 * @file        text.h
 * @version     V1.0
 * @date        2025-02-19
 * @brief       汉字显示 代码
 *              提供text_show_font和text_show_string两个函数,用于显示汉字
 ****************************************************************************************************
 * @attention
 *
 *
 * 修改说明
 * V1.0 20250213
 * 添加字库存放于SD_NAND
 *
 ****************************************************************************************************
 */

#ifndef __TEXT_H
#define __TEXT_H

#include "fonts.h"


/* 接口函数申明 */
void text_show_font(uint16_t x, uint16_t y, uint8_t *font, uint8_t size, uint8_t mode, uint16_t color);
void text_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, char *str, uint8_t size, uint8_t mode, uint8_t encode, uint16_t color);
void text_show_string_middle(uint16_t x, uint16_t y, char *str, uint8_t size, uint16_t width, uint16_t color);

#endif
