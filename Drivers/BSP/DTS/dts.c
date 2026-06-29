/**
 ****************************************************************************************************
 * @file        dts.c
 * @version     V1.0
 * @date        2025-01-13
 * @brief       DTS驱动代码
 ****************************************************************************************************
 * @attention
 * 
 * 
 ****************************************************************************************************
 */

#include "dts.h"

extern DTS_HandleTypeDef hdts;  /* DTS句柄 */

/**
 * @brief   初始化DTS
 * @param   无
 * @retval  无
 */
void dts_init(void)
{
    return;
}

/**
 * @brief   获取DTS测量结果
 * @param   HAL_DTS_Sensor: DTS传感器
 * @retval  DTS测量结果
 */
float_t dts_get_result(HAL_DTS_Sensor sensor)
{
    float_t temperature;

    /* 开始DTS传感器测量 */
    HAL_DTS_Start(&hdts, sensor);

    /* 等待DTS传感器测量结束 */
    HAL_DTS_PollForTemperature(&hdts, sensor, 1000);

    /* 获取DTS传感器测量结果 */
    HAL_DTS_GetTemperature(&hdts, sensor, &temperature);

    /* 停止DTS传感器测量 */
    HAL_DTS_Stop(&hdts, sensor);

    return temperature;
}
