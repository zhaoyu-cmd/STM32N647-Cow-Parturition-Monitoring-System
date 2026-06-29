/**
 ****************************************************************************************************
 * @file        wwdg.c
 * @version     V1.0
 * @date        2025-01-13
 * @brief       WWDG驱动代码
 ****************************************************************************************************
 * @attention
 * 
 * 
 ****************************************************************************************************
 */

#include "wwdg.h"

extern WWDG_HandleTypeDef hwwdg;    /* WWDG句柄 */

/**
 * @brief   初始化WWDG
 * @note    WWDG时钟源为pclk
 * @param   counter: 计数器值
 * @param   window: 窗口值
 * @param   prescaler: 预分频器系数
 * @arg     WWDG_PRESCALER_1  : 1分频
 * @arg     WWDG_PRESCALER_2  : 2分频
 * @arg     WWDG_PRESCALER_4  : 4分频
 * @arg     WWDG_PRESCALER_8  : 8分频
 * @arg     WWDG_PRESCALER_16 : 16分频
 * @arg     WWDG_PRESCALER_32 : 32分频
 * @arg     WWDG_PRESCALER_64 : 64分频
 * @arg     WWDG_PRESCALER_128: 128分频
 * @retval  无
 */
void wwdg_init(uint32_t counter, uint32_t window, uint32_t prescaler)
{
    UNUSED(counter);
    UNUSED(window);
    UNUSED(prescaler);

    __HAL_WWDG_ENABLE_IT(&hwwdg, WWDG_IT_EWI);
}
