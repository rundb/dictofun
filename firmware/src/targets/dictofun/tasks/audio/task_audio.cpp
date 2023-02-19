// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */
#include "task_audio.h"
#include <nrfx.h>
#include "nrf_log.h"
#include "boards.h"


#include "FreeRTOS.h"
#include "task.h"

#include "microphone_pdm.h"
#include "codec_decimate.h"
#include "audio_processor.h"

namespace audio
{

CommandQueueElement audio_command_buffer;
constexpr TickType_t audio_command_wait_passive_ticks{10};
constexpr TickType_t audio_command_wait_active_ticks{2};

static uint32_t recorded_data_size{0};
static uint32_t lost_data_size{0};

audio::microphone::PdmMicrophone<pdm_sample_size> pdm_mic{CONFIG_IO_PDM_CLK, CONFIG_IO_PDM_DATA};
using CodecInputType = audio::microphone::PdmMicrophone<pdm_sample_size>::SampleType;
using CodecOutputType = audio::codec::Sample<pdm_sample_size/decimator_codec_factor>;
audio::codec::DecimatorCodec<CodecInputType, CodecOutputType> decimator_codec{sizeof(uint16_t)};

audio::AudioProcessor<
    audio::microphone::PdmMicrophone<pdm_sample_size>::SampleType,
    CodecOutputType>
    audio_processor{pdm_mic, decimator_codec};

void microphone::isr_pdm_event_handler(const nrfx_pdm_evt_t * const p_evt)
{
    pdm_mic.pdm_event_handler(p_evt);
}

// static constexpr size_t ROTU_samples_count{100};
// static uint32_t ROTU_timestamps[ROTU_samples_count]{0};
// static uint32_t ROTU_idx{0};
// #define ROTU_REG_TIMESTAMP() {if (ROTU_idx < ROTU_samples_count) { ROTU_timestamps[ROTU_idx++] = xTaskGetTickCount();} }

void task_audio(void * context_ptr)
{
    NRF_LOG_INFO("task audio: initialized");
    Context& context = *(reinterpret_cast<Context *>(context_ptr));
    audio_processor.init();
    while (1)
    {
        const auto audio_queue_receive_status = xQueueReceive(
            context.commands_queue,
            reinterpret_cast<void *>(&audio_command_buffer),
            ((context.is_recording_active) ? 
                audio_command_wait_active_ticks : 
                audio_command_wait_passive_ticks)
        );
        if (pdPASS == audio_queue_receive_status)
        {
            if (audio_command_buffer.command_id == Command::RECORD_START)
            {
                NRF_LOG_INFO("audio: received record_start command");
                audio_processor.start();
                context.is_recording_active = true;
            }
            if (audio_command_buffer.command_id == Command::RECORD_STOP)
            {
                context.is_recording_active = false;
                audio_processor.stop();
                NRF_LOG_INFO("audio: received record_stop command. recorded %d bytes, lost %d", recorded_data_size, lost_data_size);
                // for (auto i = 0; i < ROTU_idx; ++i)
                // {
                //     NRF_LOG_DEBUG("%d %d", i, ROTU_timestamps[i]);
                //     vTaskDelay(5);
                // }
            }
        }
        const auto cyclic_call_result = audio_processor.cyclic();
        if (CyclicCallStatus::DATA_READY == cyclic_call_result)
        {
            auto& sample = audio_processor.get_last_sample();
            // TODO: push this item to the data queue
            const auto data_queue_send_result = xQueueSend(
                context.data_queue,
                reinterpret_cast<void *>(&sample),
                0);
            if (pdPASS != data_queue_send_result)
            {
                NRF_LOG_WARNING("audio: data lost %d", xTaskGetTickCount());
                // TODO: it may be needed to abort the whole record process here.
                lost_data_size += sizeof(sample);
            }
            recorded_data_size += sizeof(sample);
            // ROTU_REG_TIMESTAMP();
        }
    }
}
}
