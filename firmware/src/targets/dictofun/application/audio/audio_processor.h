// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#include "microphone.h"

// This is a set of public interfaces used for handling audio in Dictofun application.
namespace audio
{

/// @brief This class is responsible for:
/// - control of microphone
/// - memory management for microphone buffers
/// - control over usage of a codec
template <typename MicrophoneSample>
class AudioProcessor
{
public:
    explicit AudioProcessor(Microphone<MicrophoneSample>& microphone)
    : microphone_(microphone)
    {}

    void init();
    void start();
    void stop();

    AudioProcessor() = delete;
    AudioProcessor(AudioProcessor&) = delete;
    AudioProcessor(const AudioProcessor&) = delete;
    AudioProcessor(AudioProcessor&&) = delete;
    AudioProcessor(const AudioProcessor&&) = delete;

    int pdm_interrupt_calls_{0};
private:
    Microphone<MicrophoneSample>& microphone_;
    void microphone_data_ready_callback();
};

}

#include "audio_processor.hpp"
