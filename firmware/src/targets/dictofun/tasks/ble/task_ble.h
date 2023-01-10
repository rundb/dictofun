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
    QueueHandle_t status_queue;
};

enum class Command
{
    START,
    STOP,
};

struct CommandQueueElement
{
    Command command_id;
};

enum class Status
{
    OK,
    ERROR,
};

struct StatusQueueElement
{
    Command command_id;
    Status status;
};

}