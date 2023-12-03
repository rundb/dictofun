// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "task_wdt.h"
#include "nrf_drv_wdt.h"
#include "nrf_log.h"

namespace wdt
{

nrf_drv_wdt_channel_id m_channel_id;

void wdt_event_handler(void) {
    // do nothing for now. maybe store watchdog reset reason in noinitmem
}

void task_wdt(void* context)
{
    NRF_LOG_INFO("task wdg: initialized");
    // initialize watchdog
    nrf_drv_wdt_config_t config = NRF_DRV_WDT_DEAFULT_CONFIG;
    auto err_code = nrf_drv_wdt_init(&config, wdt_event_handler);
    if (NRF_SUCCESS != err_code)
    {
        NRF_LOG_ERROR("failed to initialize wdg");
    }
    err_code = nrf_drv_wdt_channel_alloc(&m_channel_id);
    if (NRF_SUCCESS != err_code)
    {
        NRF_LOG_ERROR("failed to allocate channel for wdg");
    }
    nrf_drv_wdt_enable();

    while (1) {
        // feed watchdog once a second
        nrf_drv_wdt_channel_feed(m_channel_id);

        vTaskDelay(1000);
    }
}

}
