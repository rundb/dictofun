// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "task_ble.h"
#include "nrf_log.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "task_rtc.h"

#include "ble_system.h"

namespace ble
{

BleSystem ble_system;

ble::CommandQueueElement command_buffer;
ble::BatteryDataElement batt_level_buffer;
constexpr TickType_t command_wait_slow_ticks{5U};
constexpr TickType_t command_wait_fast_ticks{1U};

struct
{
    bool is_active;
    uint8_t led_state;
} led_state_request;

void task_level_led_write_handler(uint16_t conn_handle, ble_lbs_t* p_lbs, uint8_t led_state)
{
    NRF_LOG_INFO("ble: received led write request");
    led_state_request.is_active = true;
    led_state_request.led_state = led_state;
}

/// @brief This task is responsible for all functionalities related to BLE operation.
void task_ble(void* context_ptr)
{
    Context& context{*reinterpret_cast<Context*>(context_ptr)};
    // BLE task initialization has to be delayed for 2 reasons.
    // 1. we don't need BLE immediately after startup, it doesn't carry any real-time functionality
    // 2. NVM config has to be accessed immediately after startup. It can be done using SD-aware and SD-ignorant implementations.
    //    There are 2 options: either use SD-aware and wait until SD is loaded, or use SD-ignorant immediately and switch to SD-aware
    //    after the config is loaded. I have chosen the first option, as it introduces less dependencies between tasks.
    static constexpr uint32_t ble_subsystem_startup_delay{100};
    vTaskDelay(ble_subsystem_startup_delay);
    const auto configure_result = ble_system.configure(task_level_led_write_handler);
    if(result::Result::OK != configure_result)
    {
        NRF_LOG_ERROR("task ble: init has failed");
        // Suspend the task
        vTaskSuspend(NULL);
    }

    NRF_LOG_DEBUG("task ble: initialized");

    while(1)
    {
        const auto command_receive_status = xQueueReceive(
            context.command_queue,
            reinterpret_cast<void*>(&command_buffer),
            ble_system.is_fts_active() ? command_wait_fast_ticks : command_wait_slow_ticks);
        if(pdPASS == command_receive_status)
        {
            switch(command_buffer.command_id)
            {
            case Command::START: {
                const auto start_result = ble_system.start();
                if(result::Result::OK != start_result)
                {
                    NRF_LOG_ERROR("task ble: start has failed");
                }
                else
                {
                    NRF_LOG_INFO("task ble: start");
                }
                break;
            }
            case Command::STOP: {
                const auto stop_result = ble_system.stop();
                if(result::Result::OK != stop_result)
                {
                    NRF_LOG_ERROR("task ble: stop has failed");
                }
                else
                {
                    NRF_LOG_INFO("task ble: stop");
                }
                break;
            }
            case Command::RESET_PAIRING: {
                const auto pairing_reset_result = ble_system.reset_pairing();
                if(result::Result::OK != pairing_reset_result)
                {
                    NRF_LOG_ERROR("ble: failed to reset pairing");
                }
                else
                {
                    NRF_LOG_INFO("task ble: pairing reset command succeeded");
                }
                break;
            }
            case Command::CONNECT_FS: {
                ble_system.register_fs_communication_queues(context.command_to_mem_queue,
                                                            context.status_from_mem_queue,
                                                            context.data_from_mem_queue);
                ble_system.register_keepalive_queue(context.keepalive_queue);
                ble_system.connect_fts_to_target_fs();
                break;
            }
            }
        }
        if(led_state_request.is_active)
        {
            led_state_request.is_active = false;
            RequestQueueElement cmd{Request::LED,
                                    {static_cast<uint32_t>(led_state_request.led_state)}};
            const auto command_send_status =
                xQueueSend(context.requests_queue, reinterpret_cast<void*>(&cmd), 0);
            if(pdPASS != command_send_status)
            {
                NRF_LOG_ERROR("BLE: failed to send led write request");
            }
        }

        ble_system.process();
        // process keepalive packets
        if(ble_system.has_connect_happened())
        {
            KeepaliveQueueElement evt{KeepaliveEvent::CONNECTED};
            const auto keepalive_send_result =
                xQueueSend(context.keepalive_queue, reinterpret_cast<void*>(&evt), 0);
            if(pdTRUE != keepalive_send_result)
            {
                NRF_LOG_WARNING("keepalive on-connect send has failed");
            }
        }
        if(ble_system.has_disconnect_happened())
        {
            KeepaliveQueueElement evt{KeepaliveEvent::DISCONNECTED};
            const auto keepalive_send_result =
                xQueueSend(context.keepalive_queue, reinterpret_cast<void*>(&evt), 0);
            if(pdTRUE != keepalive_send_result)
            {
                NRF_LOG_WARNING("keepalive on-disconnect send has failed");
            }
        }

        if(ble_system.is_time_update_pending())
        {
            time::DateTime current_time;
            const auto cts_get_result = ble_system.get_current_cts_time(current_time);
            if(result::Result::OK != cts_get_result)
            {
                NRF_LOG_ERROR("failed to retrieve current CTS time");
            }
            else
            {
                rtc::CommandQueueElement cmd{rtc::Command::SET_TIME,
                                             {static_cast<uint8_t>(current_time.year % 2000),
                                              current_time.month,
                                              current_time.day,
                                              current_time.hour,
                                              current_time.minute,
                                              current_time.second}};
                const auto send_result = xQueueSend(context.commands_to_rtc_queue, &cmd, 0);
                if(pdTRUE != send_result)
                {
                    // Queue operation has failed
                    NRF_LOG_WARNING("ble: set time request has failed.");
                    return;
                }
            }
        }

        const auto batt_level_receive_status = xQueueReceive(
            context.battery_to_ble_queue, reinterpret_cast<void*>(&batt_level_buffer), 0);
        if(pdPASS == batt_level_receive_status)
        {
            ble_system.set_battery_level(batt_level_buffer.batt_level);
            // TODO: utilize battery voltage too (to gather statistics on battery live across all the devices)
        }
    }
}

} // namespace ble
