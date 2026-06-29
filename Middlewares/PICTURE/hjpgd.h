/**
 ****************************************************************************************************
 * @file        hjpgd.h
 * @version     V1.0
 * @date        2025-03-18
 * @brief       驱动代码-jpeg硬件解码部分 代码
 ****************************************************************************************************
 * @attention
 *
 *
 ****************************************************************************************************
 */

#ifdef HJPEG

#ifndef __HJPGD_H
#define __HJPGD_H

#include "./JPEGCODEC/jpegcodec.h"

extern jpeg_codec_typedef dehjpgd;

/******************************************************************************************/

/* 接口函数 */
void jpeg_dma_in_callback(void);
void jpeg_dma_out_callback(void);
void jpeg_endofcovert_callback(void);
void jpeg_hdrover_callback(void);
uint8_t hjpgd_decode(char* pname);

#endif

#endif
