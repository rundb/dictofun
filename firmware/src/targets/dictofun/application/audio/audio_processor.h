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

    // This function should be periodically called from the OS context.
    // TODO: define minimal call period depending on sample size
    void cyclic();

    AudioProcessor() = delete;
    AudioProcessor(AudioProcessor&) = delete;
    AudioProcessor(const AudioProcessor&) = delete;
    AudioProcessor(AudioProcessor&&) = delete;
    AudioProcessor(const AudioProcessor&&) = delete;

private:
    Microphone<MicrophoneSample>& microphone_;
    MicrophoneSample sample_;
    void microphone_data_ready_callback();
    volatile bool is_data_frame_pending_{false};

};

}

#include "audio_processor.hpp"
