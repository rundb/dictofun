// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#pragma once

#include "FreeRTOS.h"
#include "queue.h"

namespace ble
{

void task_ble(void * context_ptr);

struct Context
{
    QueueHandle_t command_queue;
    QueueHandle_t requests_queue;
};

enum class Command
{
    START,
    STOP,
    RESET_PAIRING,
};

struct CommandQueueElement
{
    Command command_id;
};

enum class Request
{
    LED
};

struct RequestQueueElement
{
    Request request;
    static constexpr size_t max_args_count{1};
    uint32_t args[max_args_count];
};

}
