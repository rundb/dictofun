// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */
#pragma once

#include "microphone.h"
#include <cstddef>
#include <functional>

#include "nrf_drv_pdm.h"

namespace audio
{
namespace microphone
{

template <size_t SampleBufferSize>
struct PdmSample
{
    uint8_t data[SampleBufferSize];
};

// TODO: add a check that this object exists as exactly one sample in the whole system.
// TODO: implement the "rule of 5"
template <size_t SampleBufferSize>
class PdmMicrophone: public Microphone<PdmSample<SampleBufferSize>>
{
public:
    using SampleType = PdmSample<SampleBufferSize>;
    using MicrophoneType = Microphone<SampleType>;

    explicit PdmMicrophone(uint32_t clk_pin, uint32_t data_pin)
    : clk_pin_(clk_pin)
    , data_pin_(data_pin)
    {}

    void init() override;
    void start_recording() override;
    void stop_recording() override;

    result::Result get_samples(SampleType& sample) override;

    using PdmDataReadyCallback = std::function<void()>;
    void register_data_ready_callback(PdmDataReadyCallback callback) override;
    void pdm_event_handler(const nrfx_pdm_evt_t * const p_evt);

private:
    uint8_t clk_pin_;
    uint8_t data_pin_;
    static constexpr size_t buffers_count{2U};
    static constexpr size_t buffer_size{SampleBufferSize/2}; // divided by 2, as the buffer element is 2 bytes
    int16_t buffers_[buffers_count][buffer_size];
    size_t previous_buffer_index_{0};
    size_t current_buffer_index_{0};
    PdmDataReadyCallback data_ready_callback{nullptr};
};

}
}

#include "microphone_pdm.hpp"
