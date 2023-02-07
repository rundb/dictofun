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
constexpr TickType_t command_wait_ticks{5U};

struct 
{
    bool is_active;
    uint8_t led_state;
} led_state_request;

void task_level_led_write_handler(uint16_t conn_handle, ble_lbs_t * p_lbs, uint8_t led_state)
{
    NRF_LOG_INFO("ble: received led write request");
    led_state_request.is_active = true;
    led_state_request.led_state = led_state;
}

/// @brief This task is responsible for all functionalities related to BLE operation.
void task_ble(void * context_ptr)
{
    Context& context{*reinterpret_cast<Context *>(context_ptr)};
    const auto configure_result = ble_system.configure(task_level_led_write_handler);
    if (result::Result::OK != configure_result)
    {
        NRF_LOG_ERROR("task ble: init has failed");
        // Suspend the task 
        vTaskSuspend(NULL);
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
                case Command::CONNECT_FS:
                {
                    ble_system.register_fs_communication_queues(
                        &context.command_to_mem_queue, 
                        &context.status_from_mem_queue, 
                        &context.data_from_mem_queue);
                    ble_system.connect_fts_to_target_fs();
                    break;
                }
            }
        }
        if (led_state_request.is_active)
        {
            led_state_request.is_active = false;
            RequestQueueElement cmd{Request::LED, {static_cast<uint32_t>(led_state_request.led_state)}};
            const auto command_send_status = xQueueSend(
                context.requests_queue,
                reinterpret_cast<void *>(&cmd),
                0
            );
            if (pdPASS != command_send_status)
            {
                NRF_LOG_ERROR("BLE: failed to send led write request");
            }
        }

        ble_system.process();
    }
}

}
