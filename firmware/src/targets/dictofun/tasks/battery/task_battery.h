// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#pragma once

#include "FreeRTOS.h"
#include "queue.h"
#include "result.h"

namespace battery
{

/// @brief Function that implements battery task
/// @param context_ptr pointer to struct Context, passed from the main.cpp
void task_battery(void* context_ptr);

struct MeasurementsQueueElement
{
    float battery_voltage_level;
    uint8_t battery_percentage;
};

struct Context
{
    QueueHandle_t measurements_handle;
};

} // namespace battery
