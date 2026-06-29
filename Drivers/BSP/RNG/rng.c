/**
 ****************************************************************************************************
 * @file        rng.c
 * @version     V1.0
 * @date        2025-01-13
 * @brief       RNG驱动代码
 ****************************************************************************************************
 * @attention
 * 
 * 
 ****************************************************************************************************
 */

#include "rng.h"

extern RNG_HandleTypeDef hrng;  /* RNG句柄 */

/**
 * @brief   初始化随机数发生器
 * @param   无
 * @retval  初始化结果
 * @arg     0: 初始化成功
 * @arg     1: 初始化失败
 */
uint8_t rng_init(void)
{
    return 0;
}

/**
 * @brief   获取随机数
 * @param   无
 * @retval  随机数
 */
uint32_t rng_get_random(void)
{
    uint32_t random;

    if (HAL_RNG_GenerateRandomNumber(&hrng, &random) != HAL_OK)
    {
        return 0;
    }

    return random;
}

/**
 * @brief   获取指定范围的随机数
 * @param   min: 指定范围的最小值
 * @param   max: 指定范围的最大值
 * @retval  随机数
 */
int32_t rng_get_random_range(int32_t min, int32_t max)
{
    uint32_t random;

    if (HAL_RNG_GenerateRandomNumber(&hrng, &random) != HAL_OK)
    {
        return 0;
    }

    return ((int32_t)random % (max - min + 1) + min);
}
