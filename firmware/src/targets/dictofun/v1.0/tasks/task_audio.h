#pragma once

#include <drv_audio.h>
#include <stdint.h>

void audio_init();
void audio_start_record();
void audio_stop_record();
void audio_frame_handle();
void audio_frame_cb(drv_audio_frame_t * frame);
size_t audio_get_record_size();