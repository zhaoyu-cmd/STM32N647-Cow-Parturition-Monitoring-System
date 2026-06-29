/**
 ****************************************************************************************************
 * @file        app_netxduo.h
 * @version     V1.0
 * @date        2025-01-13
 * @brief       app_netxduo.h文件
 ****************************************************************************************************
 * @attention
 * 
 * 
 ****************************************************************************************************
 */

#ifndef __APP_NETXDUO_H
#define __APP_NETXDUO_H

#include "nx_api.h"

UINT app_netxduo_init(VOID *memory_ptr);

/**
 * Post a detection alert to the network notifier (lock-free, callable from
 * any ThreadX thread). Non-blocking: drops the alert if the internal queue
 * is full. `class_name` must point to memory that stays valid forever
 * (e.g. an entry in a `static const char *[]` table).
 */
void app_netxduo_post_alert(const char *class_name, float conf);

#endif
