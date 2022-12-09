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

template <size_t SampleBufferSize>
class PdmMicrophone: public Microphone<PdmSample<SampleBufferSize>>
{
public:
    using SampleType = PdmSample<SampleBufferSize>;
    using MicrophoneType = Microphone<SampleType>;

    void start_recording() override;
    void stop_recording() override;

    result::Result get_samples(SampleType& samples) override;

    using PdmDataReadyCallback = std::function<void()>;
    void register_data_ready_callback(PdmDataReadyCallback callback) override;
private:
    static constexpr size_t buffers_count_{2U};
};

}
}

#include "microphone_pdm.hpp"
