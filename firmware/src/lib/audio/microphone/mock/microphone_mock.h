// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#pragma once

#include "microphone.h"
#include <stdint.h>

namespace audio
{

struct MockAudioSample
{
    uint16_t data[8U];
};

class MicrophoneMock : public Microphone<MockAudioSample>
{
public:
    void init() override;
    void start_recording() override;
    void stop_recording() override;
    result::Result get_samples(MockAudioSample& samples) override;

    void register_data_ready_callback(DataReadyCallback callback) override;
};

} // namespace audio
