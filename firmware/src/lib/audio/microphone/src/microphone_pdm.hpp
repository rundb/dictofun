// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */
#pragma once

#include "nrf_drv_pdm.h"
#include "boards.h"
#include "nrf_log.h"

namespace audio
{
namespace microphone
{

// It's a necessary evil here. Since particular template instantiation is unknown at this location, and 
// callback has to be a C-style function for Nordic library, this function has to be implemented next to 
// the instantiation of the microphone class. It has to call PdmMicrophone<>::pdm_event_handler()
extern void isr_pdm_event_handler(const nrfx_pdm_evt_t * const p_evt);

template <size_t SampleBufferSize>
void PdmMicrophone<SampleBufferSize>::init() 
{
    static constexpr uint8_t CONFIG_AUDIO_PDM_GAIN {0x38};
    
    nrf_drv_pdm_config_t pdm_cfg = NRF_DRV_PDM_DEFAULT_CONFIG(clk_pin_, data_pin_);
    pdm_cfg.gain_l          = CONFIG_AUDIO_PDM_GAIN;
    pdm_cfg.gain_r          = CONFIG_AUDIO_PDM_GAIN;

    auto rc =  nrf_drv_pdm_init(&pdm_cfg, isr_pdm_event_handler);
    if (NRF_SUCCESS != rc)
    {
        NRF_LOG_ERROR("pdm: init error (%d)", rc);
    }
}

template <size_t SampleBufferSize>
void PdmMicrophone<SampleBufferSize>::pdm_event_handler(const nrfx_pdm_evt_t * const p_evt)
{
    if (p_evt->buffer_requested)
    {
        previous_buffer_index_ = current_buffer_index_;
        current_buffer_index_ = (current_buffer_index_ + 1) % buffers_count;
        nrf_drv_pdm_buffer_set(buffers_[current_buffer_index_], buffer_size);
        data_ready_callback();
    }
    if (p_evt->buffer_released != nullptr)
    {
        _released_buffer_ptr = p_evt->buffer_released;
    }
}

template <size_t SampleBufferSize>
void PdmMicrophone<SampleBufferSize>::start_recording() 
{
    nrf_drv_pdm_buffer_set(buffers_[current_buffer_index_], buffer_size);

    const auto start_result = nrf_drv_pdm_start();
    if (start_result != 0)
    {
        NRF_LOG_ERROR("pdm: start has failed, result = %d", start_result);
    }
}

template <size_t SampleBufferSize>
void PdmMicrophone<SampleBufferSize>::stop_recording() 
{
    nrf_drv_pdm_stop();
}

template <size_t SampleBufferSize>
result::Result PdmMicrophone<SampleBufferSize>::get_samples(SampleType& sample) 
{
    memcpy(sample.data, _released_buffer_ptr, SampleBufferSize);
    return result::Result::OK;
}

template <size_t SampleBufferSize>
void PdmMicrophone<SampleBufferSize>::register_data_ready_callback(PdmDataReadyCallback callback)
{   
    data_ready_callback = callback;
}


}
}
