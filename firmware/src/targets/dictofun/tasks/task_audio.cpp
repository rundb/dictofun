// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */
#include "task_audio.h"
#include "task_state.h"
#include <nrfx.h>
#include "nrf_log.h"
#include "simple_fs.h"

#include "FreeRTOS.h"
#include "task.h"

#include "app_timer.h"

drv_audio_frame_t * pending_frame = NULL;
#define SPI_XFER_DATA_STEP 0x100

filesystem::File * _current_audio_file{nullptr};
uint32_t start_time, end_time, frames_count, total_data, written_data;
uint32_t valid_writes_counter = 0;
uint32_t invalid_writes_counter = 0;


void task_audio(void *)
{
    while (1)
    {
        vTaskSuspend(NULL); // Suspend myself
    }
}

void audio_init()
{
    drv_audio_init(audio_frame_cb);   
}

void audio_start_record(filesystem::File& file)
{
    _current_audio_file = &file;
    drv_audio_transmission_enable();

    start_time = app_timer_cnt_get();
    frames_count = 0;
    total_data = 0;
    written_data = 0;

    valid_writes_counter = 0;
    invalid_writes_counter = 0;
}

// TODO: replace with setting a flag that stops the recording in 
//       the interrupt. Thus we can assure that last chunk is saved.
result::Result audio_stop_record()
{
    drv_audio_transmission_disable();
    end_time = app_timer_cnt_get();
    float data_rate = (float)total_data * 1000 / float(end_time - start_time);
    float store_rate = (float)written_data * 1000 / float(end_time - start_time);
    NRF_LOG_INFO("audio: timespan=%d, data rate = %d, store rate = %d", end_time - start_time, (int)data_rate, (int)store_rate);

    _current_audio_file = nullptr;

    return (invalid_writes_counter > valid_writes_counter) ? result::Result::ERROR_GENERAL : result::Result::OK;
}

void audio_frame_handle()
{
    if(pending_frame) 
    {
        const int data_size = pending_frame->buffer_occupied;
        NRFX_ASSERT(data_size <= SPI_XFER_DATA_STEP);
        NRFX_ASSERT(application::getApplicationState() == application::AppSmState::RECORD);
        
        const auto res = filesystem::write(*_current_audio_file, pending_frame->buffer, data_size);
        if (res != result::Result::OK)
        {
            invalid_writes_counter++;
        }
        else
        {
            valid_writes_counter++;
        }
        // TODO: assert on wrong res
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
