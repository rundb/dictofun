// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */
#pragma once

#include "boards.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "timers.h"

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
};

void record_end_callback(TimerHandle_t timer);

}
