/**
 ****************************************************************************************************
 * @file        stm32n6xx_hal_timebase_threadx.c
 * @version     V1.0
 * @date        2025-01-13
 * @brief       stm32n6xx_hal_timebase_threadx.c文件
 ****************************************************************************************************
 * @attention
 * 
 * 
 ****************************************************************************************************
 */

#include "stm32n6xx_hal.h"
#include "tx_api.h"

HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    UNUSED(TickPriority);

    return 0;
}

uint32_t HAL_GetTick(void)
{
    return tx_time_get();
}

void HAL_Delay(uint32_t Delay)
{
    tx_thread_sleep(Delay);
}
