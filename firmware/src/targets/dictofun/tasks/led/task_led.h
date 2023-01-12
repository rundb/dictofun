// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */
#pragma once

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"


namespace led
{
// This task is responsible for the whole LED management.
// Separation of this logic into a separate unit allows us to
// concentrate all indication-dependencies inside one unit.

enum class State
{
    OFF,
    // SLOW_BLINKING,
    // FAST_BLINKING,
    ON
};

enum class Color
{
    RED = 0,
    GREEN = 1,
    BLUE = 2,
    COUNT = 3
};

constexpr Color user_color{Color::GREEN};

struct CommandQueueElement
{
    Color color;
    State state;
};

struct Context
{
    QueueHandle_t commands_queue{nullptr};
};


void task_led(void * context_ptr);

}