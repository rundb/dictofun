// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */
#pragma once

#include "boards.h"
#include "FreeRTOS.h"
#include "queue.h"

namespace systemstate
{
void task_system_state(void * context);

struct Context
{
    QueueHandle_t cli_commands_handle{nullptr};
    QueueHandle_t cli_status_handle{nullptr};
    // TODO: extend with rest of command queues controlled by SystemState
    QueueHandle_t audio_commands_handle{nullptr};
    QueueHandle_t audio_status_handle{nullptr};
};

}

namespace application
{

enum class AppSmState
{
    INIT,
    PREPARE,
    RECORD_START,
    RECORD,
    RECORD_FINALIZATION,
    CONNECT,
    TRANSFER,
    DISCONNECT,
    FINALIZE,
    SHUTDOWN,
    RESTART
};

AppSmState getApplicationState();

void application_init();
void application_cyclic();
} // namespace application
