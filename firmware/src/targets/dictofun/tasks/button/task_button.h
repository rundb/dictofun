// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#pragma once

#include "FreeRTOS.h"
#include "queue.h"

namespace button
{

void task_button(void* context);

struct Context
{
    QueueHandle_t events_handle{nullptr};
    QueueHandle_t commands_handle{nullptr};
    QueueHandle_t response_handle{nullptr};
};

enum class ButtonState
{
    RELEASED,
    PRESSED,
    UNKNOWN,
};

enum class Event
{
    SINGLE_PRESS_ON,
    SINGLE_PRESS_OFF,
    NONE,
};

enum class Command
{
    REQUEST_STATE,
};

// Communication state->button
struct RequestQueueElement
{
    Command command;
};

struct ResponseQueueElement
{
    ButtonState state;
};

struct EventQueueElement
{
    Event event;
    ButtonState state;
};

} // namespace button
