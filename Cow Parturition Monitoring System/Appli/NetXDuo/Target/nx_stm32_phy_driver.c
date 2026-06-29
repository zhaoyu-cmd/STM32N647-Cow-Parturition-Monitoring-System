/**
 ****************************************************************************************************
 * @file        nx_stm32_phy_driver.c
 * @version     V1.0
 * @date        2025-01-13
 * @brief       nx_stm32_phy_driver.c文件
 ****************************************************************************************************
 * @attention
 * 
 * 
 ****************************************************************************************************
 */

#include "nx_stm32_phy_driver.h"
#include "nx_stm32_eth_config.h"
#include "./YT8512C/yt8512c.h"

int32_t nx_eth_phy_init(void)
{
    yt8512c_init();

    return ETH_PHY_STATUS_OK;
}

int32_t nx_eth_phy_get_link_state(void)
{
    uint8_t link_state;

    link_state = yt8512c_get_link_state();
    if (link_state == YT8512C_LINK_STATE_DOWN)
    {
        return ETH_PHY_STATUS_LINK_DOWN;
    }
    else if (link_state == YT8512C_LINK_STATE_100MBITS_FULLDUPLEX)
    {
        return ETH_PHY_STATUS_100MBITS_FULLDUPLEX;
    }
    else if (link_state == YT8512C_LINK_STATE_100MBITS_HALFDUPLEX)
    {
        return ETH_PHY_STATUS_100MBITS_HALFDUPLEX;
    }
    else if (link_state == YT8512C_LINK_STATE_10MBITS_FULLDUPLEX)
    {
        return ETH_PHY_STATUS_10MBITS_FULLDUPLEX;
    }
    else if (link_state == YT8512C_LINK_STATE_10MBITS_HALFDUPLEX)
    {
        return ETH_PHY_STATUS_10MBITS_HALFDUPLEX;
    }
    else if (link_state == YT8512C_LINK_STATE_AUTONEGO_NOTDONE)
    {
        return ETH_PHY_STATUS_AUTONEGO_NOT_DONE;
    }
    else
    {
        return ETH_PHY_STATUS_LINK_ERROR;
    }
}

nx_eth_phy_handle_t nx_eth_phy_get_handle(void)
{
    return NULL;
}
