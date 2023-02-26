// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */
#pragma once

#include <stdint.h>
#include "result.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "codec.h"

namespace audio
{

// TODO: find a more appropriate location for this configuration parameter 
constexpr size_t pdm_sample_size{256};
constexpr size_t decimator_codec_factor{1};
constexpr size_t adpcm_factor{4};
constexpr uint32_t pdm_sampling_frequency{16000};

using CodecDecimatorOutputType = audio::codec::Sample<pdm_sample_size/decimator_codec_factor>;
using CodecAdpcmOutputType = audio::codec::Sample<pdm_sample_size/adpcm_factor>;

using CodecOutputType = CodecAdpcmOutputType; 

// using CodecOutputType = CodecDecimatorOutputType;

/// @brief Function that implements audio task
/// @param context_ptr pointer to struct Context, passed from the main.cpp
void task_audio(void * context_ptr);

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
    bool is_recording_active{false};
    
    QueueHandle_t commands_queue{nullptr};
    QueueHandle_t status_queue{nullptr};

    QueueHandle_t data_queue{nullptr};
};

}
