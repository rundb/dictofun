/*
 * Copyright (c) 2021 Roman Turkin 
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "tasks/task_audio.h"
#include "tasks/task_state.h"
#include <nrfx.h>
#include "lfs.h"
#include "nrf_log.h"

#include "app_timer.h"

drv_audio_frame_t * pending_frame = NULL;
#define SPI_XFER_DATA_STEP 0x100

lfs_t * _audio_fs{nullptr};
lfs_file_t * _audio_record_file{nullptr};

uint32_t start_time, end_time, frames_count, total_data, written_data;

void audio_init()
{
    drv_audio_init(audio_frame_cb);   
}

void audio_start_record(lfs_t * fs, lfs_file_t * file)
{
    _audio_record_file = file;
    _audio_fs = fs;
    drv_audio_transmission_enable();

    start_time = app_timer_cnt_get();
    frames_count = 0;
    total_data = 0;
    written_data = 0;
}

// TODO: replace with setting a flag that stops the recording in 
//       the interrupt. Thus we can assure that last chunk is saved.
void audio_stop_record()
{
    drv_audio_transmission_disable();
    end_time = app_timer_cnt_get();
    float data_rate = (float)total_data * 1000 / float(end_time - start_time);
    float store_rate = (float)written_data * 1000 / float(end_time - start_time);
    NRF_LOG_INFO("audio: timespan=%d, data rate = %d, store rate = %d", end_time - start_time, (int)data_rate, (int)store_rate);
}

void audio_frame_handle()
{
    if(pending_frame) 
    {
        const int data_size = pending_frame->buffer_occupied;
        NRFX_ASSERT(data_size < SPI_XFER_DATA_STEP);
        NRFX_ASSERT(application::getApplicationState() == application::AppSmState::RECORD);
        
        lfs_file_write(_audio_fs, _audio_record_file, pending_frame->buffer, data_size);
        written_data += data_size;

        pending_frame->buffer_free_flag = true;
        pending_frame->buffer_occupied = 0;
        pending_frame = NULL;
    }
}

uint32_t frame_counter{0};
uint32_t invalid_frame_counter{0};
void audio_frame_cb(drv_audio_frame_t * frame)
{
    frames_count++;
    total_data += frame->buffer_occupied;

    //NRFX_ASSERT(NULL == pending_frame);
    if (NULL != pending_frame) 
    {
        frame->buffer_free_flag = true;
        frame->buffer_occupied = 0;
        invalid_frame_counter++;
        return;
    }
    frame_counter++;
    pending_frame = frame;
}
