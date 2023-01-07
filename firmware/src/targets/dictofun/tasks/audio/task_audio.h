// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */
#pragma once

#include <stdint.h>
#include "simple_fs.h"
#include "result.h"

#include "FreeRTOS.h"
#include "queue.h"

namespace audio
{
/// @brief Function that implements audio task
/// @param context_ptr pointer to struct Context, passed from the main.cpp
void task_audio(void * context_ptr);

enum class Command
{
    RECORD_START,
    RECORD_STOP,
};

enum class Status
{
    OK,
    ERROR,
};

struct CommandQueueElement
{
    Command command_id;
};

struct StatusQueueElement
{
    Command command_id;
    Status status;
};

struct Context
{
    QueueHandle_t commands_queue{nullptr};
    QueueHandle_t status_queue{nullptr};
    // TODO: add data queue as well
};

}
