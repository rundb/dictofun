// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */
#pragma once

#include "FreeRTOS.h"
#include "boards.h"
#include "queue.h"
#include "timers.h"

#include "nvconfig.h"

namespace systemstate
{
void task_system_state(void* context);

struct Context
{
    QueueHandle_t cli_commands_handle{nullptr};
    QueueHandle_t cli_status_handle{nullptr};

    QueueHandle_t audio_commands_handle{nullptr};
    QueueHandle_t audio_status_handle{nullptr};

    QueueHandle_t memory_commands_handle{nullptr};
    QueueHandle_t memory_status_handle{nullptr};

    QueueHandle_t audio_tester_commands_handle{nullptr};

    QueueHandle_t ble_commands_handle{nullptr};
    QueueHandle_t ble_requests_handle{nullptr};
    QueueHandle_t ble_keepalive_handle{nullptr};

    QueueHandle_t led_commands_handle{nullptr};

    QueueHandle_t button_events_handle{nullptr};

    QueueHandle_t battery_measurements_handle{nullptr};

    TimerHandle_t record_timer_handle{nullptr};

    bool is_record_active{false};
    bool _should_record_be_stored{false};
    bool _is_ble_system_active{false};

    struct Timestamps
    {
        uint32_t last_record_start_timestamp{timestamp_uninitialized_value};
        uint32_t last_record_end_timestamp{0};
        uint32_t last_ble_activity_timestamp{timestamp_uninitialized_value};
        uint32_t ble_disconnect_event_timestamp{timestamp_uninitialized_value};

        bool has_start_timestamp_been_updated()
        {
            return last_record_start_timestamp != timestamp_uninitialized_value;
        }
        bool has_disconnect_timestamp_been_updated()
        {
            return ble_disconnect_event_timestamp != timestamp_uninitialized_value;
        }
        bool has_ble_timestamp_been_updated()
        {
            return last_ble_activity_timestamp != timestamp_uninitialized_value;
        }

    private:
        static constexpr uint32_t timestamp_uninitialized_value{0xaf1bfb98};
    };
    Timestamps timestamps;
};

void record_end_callback(TimerHandle_t timer);

void launch_cli_command_record(Context& context,
                               const uint32_t duration,
                               bool should_store_the_record);
void launch_cli_command_memory_test(Context& context, const uint32_t test_id);
void launch_cli_command_ble_operation(Context& context, const uint32_t command_id);
void launch_cli_command_system(Context& context, const uint32_t command_id);
void launch_cli_command_opmode(Context& context,
                               const uint32_t mode_id,
                               application::NvConfig& nvc);
void launch_cli_command_led(Context& context, const uint32_t color_id, const uint32_t mode_id);

void load_nvconfig(Context& context);
application::Mode get_operation_mode();
bool is_record_start_by_cli_allowed(Context& context);
result::Result launch_record_timer(const TickType_t record_duration, Context& context);
void shutdown_ldo();
void configure_power_latch();
void process_timeouts(Context& context);
result::Result enable_ble_subsystem(Context& context);
result::Result disable_ble_subsystem(Context& context);

result::Result request_record_creation(const Context& context);
result::Result request_record_start(const Context& context);
result::Result request_record_stop(const Context& context);
result::Result request_record_closure(const Context& context);

} // namespace systemstate
