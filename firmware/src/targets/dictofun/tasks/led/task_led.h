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
    OFF = 0,
    SLOW_GLOW = 1,
    FAST_GLOW = 2,
    FAST_GLOW_ALT = 3, // Intention: make a saw-signal
    ON = 4,

    COUNT,
};

enum class Color
{
    RED = 0,
    GREEN = 1,
    DARK_BLUE = 2,
    PURPLE = 3,
    YELLOW = 4,
    BRIGHT_BLUE = 5,
    WHITE = 6,
    COUNT
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

void task_led(void* context_ptr);

} // namespace led