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

ble::CommandQueueElement command_buffer;
constexpr TickType_t command_wait_ticks{10U};

/// @brief This task is responsible for all functionalities related to BLE operation.
void task_ble(void * context_ptr)
{
    Context& context{*reinterpret_cast<Context *>(context_ptr)};
    const auto configure_result = ble_system.configure();
    if (result::Result::OK != configure_result)
    {
        NRF_LOG_ERROR("task ble: init has failed");
        while(1) {vTaskDelay(100000);}
    }

    NRF_LOG_INFO("task ble: initialized");
    while(1)
    {
        const auto command_receive_status = xQueueReceive(
            context.command_queue,
            reinterpret_cast<void *>(&command_buffer),
            command_wait_ticks
        );
        if (pdPASS == command_receive_status)
        {
            switch (command_buffer.command_id)
            {
                case Command::START:
                {
                    const auto start_result = ble_system.start();
                    if (result::Result::OK != start_result)
                    {
                        NRF_LOG_ERROR("task ble: start has failed");
                    }
                    else
                    {
                        NRF_LOG_INFO("task ble: start");
                    }
                    break;
                }
                case Command::STOP:
                {
                    const auto stop_result = ble_system.stop();
                    if (result::Result::OK != stop_result)
                    {
                        NRF_LOG_ERROR("task ble: stop has failed");
                    }
                    else
                    {
                        NRF_LOG_INFO("task ble: stop");
                    }
                    break;
                }
                case Command::RESET_PAIRING:
                {
                    NRF_LOG_ERROR("task ble: pairing reset is not yet implemented");
                    break;
                }
            }
        }
    }
}

}
