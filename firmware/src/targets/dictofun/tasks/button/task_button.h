// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#pragma once

#include "FreeRTOS.h"
#include "queue.h"

namespace button
{

void task_button(void * context);

struct Context
{
    QueueHandle_t events_handle{nullptr};
};

enum class ButtonState
{
    RELEASED,
    PRESSED,
};

enum class Event
{
    SINGLE_PRESS_ON,
    SINGLE_PRESS_OFF,
};

struct EventQueueElement
{
    Event event;
};

}
