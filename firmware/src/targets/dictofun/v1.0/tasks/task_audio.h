#pragma once

#include <drv_audio.h>

void audio_init();
void audio_frame_handle();
void audio_frame_cb(drv_audio_frame_t * frame);