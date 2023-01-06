// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */
#pragma once

#include "drv_audio.h"
#include <stdint.h>
#include "simple_fs.h"
#include "result.h"

#include "FreeRTOS.h"
#include "queue.h"

namespace audio
{
void task_audio(void *);

enum class Command
{
    RECORD_START,
    RECORD_STOP,
};

enum class Status
{
    OK,
    ERROR,
};

struct CommandQueueElement
{
    Command command_id;
};

struct StatusQueueElement
{
    Command command_id;
    Status status;
};

struct Context
{
    QueueHandle_t commands_queue{nullptr};
    QueueHandle_t status_queue{nullptr};
    // TODO: add data queue as well
};

void audio_init();
void audio_start_record(filesystem::File& file);
result::Result audio_stop_record();
void audio_frame_handle();
void audio_frame_cb(drv_audio_frame_t * frame);
}