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
#include <spi_access.h>
#include <nrfx.h>
#include "lfs.h"

#include "app_timer.h"

drv_audio_frame_t * pending_frame = NULL;
lfs_file_t current_file;

#define SPI_XFER_DATA_STEP 0x100
#define CURRENT_RECORD_FILE_NAME "record.wav"

lfs_t * _fs{nullptr};

void audio_init(lfs_t * fs)
{
    _fs = fs;
    drv_audio_init(audio_frame_cb);   
}

void audio_start_record()
{
    const auto res = lfs_file_open(_fs, &current_file, CURRENT_RECORD_FILE_NAME, LFS_O_WRONLY | LFS_O_CREAT);
    if (res)
    {
        // Failed to open file.
        while(1);
    }
    // write first 1 kB of data. by doing so, we pass the slow start of file write.
    uint8_t data[256] = {0};
    lfs_file_write(_fs, &current_file, data, 256);
    drv_audio_transmission_enable();
}

// TODO: replace with setting a flag that stops the recording in 
//       the interrupt. Thus we can assure that last chunk is saved.
void audio_stop_record()
{
    drv_audio_transmission_disable();
    lfs_file_close(_fs, &current_file);
}

void audio_frame_handle()
{
    if(pending_frame) 
    {
        const int data_size = pending_frame->buffer_occupied;
        NRFX_ASSERT(data_size < SPI_XFER_DATA_STEP);
        NRFX_ASSERT(application::getApplicationState() == application::AppSmState::RECORD);

        lfs_file_write(_fs, &current_file, pending_frame->buffer, data_size);

        pending_frame->buffer_free_flag = true;
        pending_frame->buffer_occupied = 0;
        pending_frame = NULL;
    }
}

uint32_t frame_counter{0};
uint32_t invalid_frame_counter{0};
void audio_frame_cb(drv_audio_frame_t * frame)
{
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

// In fact, 2x of size is returned - as we write only half of the available memory
size_t audio_get_record_size()
{
    //lfs_stat(lfs_t *lfs, const char *path, struct lfs_info *info);
    struct lfs_info info;
    const auto res = lfs_stat(_fs, CURRENT_RECORD_FILE_NAME, &info);
    if (res != 0)
    {
      return 0;
    }
    return info.size;
}
