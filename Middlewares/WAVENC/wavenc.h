/**
 ****************************************************************************************************
 * @file        wavenc.h
 * @version     V1.0
 * @date        2025-01-13
 * @brief       WAV编码库
 ****************************************************************************************************
 * @attention
 * 
 * 
 ****************************************************************************************************
 */

#ifndef __WAVENC_H
#define __WAVENC_H

#include "main.h"

typedef enum
{
    WAVENC_OK = 0,
    WAVENC_ERROR = 1,
} wavenc_status_t;

typedef struct wavenc_stream_configure
{
    struct
    {
        uint32_t buffer_size;
        void *data;
    } stream;

    uint16_t channels;
    uint32_t sample_rate;
    uint16_t bits_per_sample;
} wavenc_configure_t;

typedef struct wavenc_stream
{
    struct stream_buffer {
        uint8_t *buffer_base;
        uint8_t *buffer_writer;
        uint8_t *buffer_end;
    } buffer[2];

    uint32_t buffer_current_index;

    void *data;
} wavenc_stream_t;

typedef struct wavenc
{
    wavenc_stream_t stream;

    uint16_t channels;
    uint32_t sample_rate;
    uint32_t bit_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;

    uint32_t data_size;
} wavenc_t;

wavenc_status_t wavenc_init(wavenc_t *wavenc, wavenc_configure_t *configure);
wavenc_status_t wavenc_deinit(wavenc_t *wavenc);

wavenc_status_t wavenc_stream_init(wavenc_stream_t *stream, uint32_t buffer_size, void *data);
wavenc_status_t wavenc_stream_deinit(wavenc_stream_t *stream);
wavenc_status_t wavenc_stream_flush_current_buffer(wavenc_stream_t *stream);
wavenc_status_t wavenc_stream_write_byte(wavenc_stream_t *stream, uint8_t *data);
wavenc_status_t wavenc_stream_input(wavenc_stream_t *stream);
uint8_t *wavenc_stream_get_buffer(wavenc_stream_t *stream);

#endif
