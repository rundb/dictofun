// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "task_ble.h"
#include "nrf_log.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

namespace ble
{

void task_ble(void * context_ptr)
{
    NRF_LOG_INFO("task ble: initialized");
    while(1)
    {
        vTaskDelay(1000);
    }
}

}