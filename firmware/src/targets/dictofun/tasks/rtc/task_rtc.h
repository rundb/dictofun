// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */
#pragma once

#include "FreeRTOS.h"
#include "queue.h"
#include "timers.h"

namespace rtc
{
void task_rtc(void* context);

struct Context
{
    QueueHandle_t command_queue{nullptr};
    QueueHandle_t response_queue{nullptr};
};

/// @brief  Commands that can be sent to the RTC task.
/// Commands can be sent from different contexts. Since this use-case suggests that
/// get- and set- accesses are clearly separated in time, only one command and response
/// queues are being used (so commands is a MPSC and response is SPMC, with access separation by time)
enum class Command : uint8_t
{
    GET_TIME,
    SET_TIME,
};

enum class Status : uint8_t
{
    OK,
    ERROR,
};

// Time/Date consists of following elements:
// year (uint8_t), month (uint8_t), day (uint8_t),
// hh, mm and ss, 1 byte each.
// In total 6 bytes
static constexpr uint32_t date_size{6};

struct CommandQueueElement
{
    Command command_id;
    uint8_t content[date_size];
};

struct ResponseQueueElement
{
    Status status;
    uint8_t content[date_size];
};

} // namespace rtc
