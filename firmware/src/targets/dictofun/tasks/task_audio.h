// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */
#pragma once

#include <drv_audio.h>
#include <stdint.h>
#include "simple_fs.h"
#include "result.h"

void audio_init();
void audio_start_record(filesystem::File& file);
result::Result audio_stop_record();
void audio_frame_handle();
void audio_frame_cb(drv_audio_frame_t * frame);