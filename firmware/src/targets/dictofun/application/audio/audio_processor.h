// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#include "microphone.h"
#include "codec.h"

// This is a set of public interfaces used for handling audio in Dictofun application.
namespace audio
{

enum class CyclicCallStatus
{
    NO_ACTION,
    DATA_READY,
    ERROR,
};

/// @brief This class is responsible for:
/// - control of microphone
/// - memory management for microphone buffers
/// - control over usage of a codec
template <typename MicrophoneSample, typename CodecOutputSample>
class AudioProcessor
{
public:
    explicit AudioProcessor(
        Microphone<MicrophoneSample>& microphone, 
        codec::Codec<MicrophoneSample, CodecOutputSample>& codec)
    : microphone_(microphone)
    , codec_(codec)
    {}

    void init();
    void start();
    void stop();

    // This function should be periodically called from the OS context.
    // TODO: define minimal call period depending on sample size
    CyclicCallStatus cyclic();

    // TODO: replace this sample with after-codec one when it's available
    CodecOutputSample& get_last_sample() { return processed_sample_; }

    AudioProcessor() = delete;
    AudioProcessor(AudioProcessor&) = delete;
    AudioProcessor(const AudioProcessor&) = delete;
    AudioProcessor(AudioProcessor&&) = delete;
    AudioProcessor(const AudioProcessor&&) = delete;

private:
    Microphone<MicrophoneSample>& microphone_;
    MicrophoneSample sample_;
    CodecOutputSample processed_sample_;
    codec::Codec<MicrophoneSample, CodecOutputSample>& codec_;
    void microphone_data_ready_callback();
    volatile bool is_data_frame_pending_{false};

};

}

#include "audio_processor.hpp"
