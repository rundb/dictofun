// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */
#pragma once

#include "boards.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "timers.h"

#include "nvconfig.h"

namespace systemstate
{
void task_system_state(void * context);

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

    QueueHandle_t led_commands_handle{nullptr};

    QueueHandle_t button_events_handle{nullptr};

    TimerHandle_t record_timer_handle{nullptr};

    bool is_record_active{false};
    bool _should_record_be_stored{false};
    bool _is_ble_system_active{false};
};

void record_end_callback(TimerHandle_t timer);

void launch_cli_command_record(Context& context, const uint32_t duration, bool should_store_the_record);
void launch_cli_command_memory_test(Context& context, const uint32_t test_id);
void launch_cli_command_ble_operation(Context& context, const uint32_t command_id);
void launch_cli_command_system(Context& context, const uint32_t command_id);
void launch_cli_command_opmode(Context& context, const uint32_t mode_id, application::NvConfig& nvc);

application::Mode get_operation_mode();

bool is_record_start_by_cli_allowed(Context& context);

result::Result launch_record_timer(const TickType_t record_duration, Context& context);

void shutdown_ldo();

result::Result enable_ble_subsystem(Context& context);

}
