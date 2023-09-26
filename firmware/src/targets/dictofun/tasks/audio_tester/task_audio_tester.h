// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */
#pragma once

#include "FreeRTOS.h"
#include "queue.h"

namespace audio
{
namespace tester
{
void task_audio_tester(void* context_ptr);

struct ControlQueueElement
{
    bool should_enable_tester;
};

struct Context
{
    QueueHandle_t data_queue{nullptr};
    QueueHandle_t commands_queue{nullptr};
};

} // namespace tester
} // namespace audio
