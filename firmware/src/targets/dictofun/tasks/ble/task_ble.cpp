// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "task_ble.h"
#include "nrf_log.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "ble_system.h"

namespace ble
{

BleSystem ble_system;

/// @brief This task is responsible for all functionalities related to BLE operation.
void task_ble(void * context_ptr)
{
    const auto configure_result = ble_system.configure();
    if (result::Result::OK != configure_result)
    {
        NRF_LOG_ERROR("task ble: init has failed");
        while(1) {vTaskDelay(100000);}
    }

    const auto start_result = ble_system.start();
    if (result::Result::OK != start_result)
    {
        NRF_LOG_ERROR("task ble: start has failed");
        while(1) {vTaskDelay(100000);}
    }

    NRF_LOG_INFO("task ble: initialized");
    while(1)
    {
        vTaskDelay(1000);
    }
}

}
