// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#pragma once

#include "FreeRTOS.h"
#include "queue.h"

namespace memory
{
bool isMemoryErased();

void task_memory(void * context_ptr);

enum class Command
{
    LAUNCH_TEST_1,
    LAUNCH_TEST_2,
    LAUNCH_TEST_3,
    FORMAT_CONFIG,
    OPEN_FILE_FOR_WRITE,
    CLOSE_FILE_FOR_WRITE,
    OPEN_FILE_FOR_READ,
    CLOSE_FILE_FOR_READ,
    INVALIDATE_FILE,
};

enum class Status
{
    OK,
    ERROR_BUSY,
    ERROR_GENERAL,
};

struct CommandQueueElement
{
    Command command_id;
    static constexpr size_t max_arguments{2};
    uint32_t args[max_arguments];
};

struct StatusQueueElement
{
    Command command_id;
    Status status;
};

struct Context
{
    QueueHandle_t audio_data_queue {nullptr};
    QueueHandle_t command_queue {nullptr};
    QueueHandle_t status_queue {nullptr};
};

}
