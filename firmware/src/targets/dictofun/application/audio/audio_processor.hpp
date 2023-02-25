// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */
#pragma once

namespace audio
{

template <typename MicrophoneSample, typename CodecOutputSample>
void AudioProcessor<MicrophoneSample, CodecOutputSample>::init()
{
    microphone_.init();
    microphone_.register_data_ready_callback(
        std::bind(&AudioProcessor<MicrophoneSample, CodecOutputSample>::microphone_data_ready_callback, this));
}

template <typename MicrophoneSample, typename CodecOutputSample>
void AudioProcessor<MicrophoneSample, CodecOutputSample>::start()
{
    codec_.start();
    microphone_.start_recording();
}

template <typename MicrophoneSample, typename CodecOutputSample>
void AudioProcessor<MicrophoneSample, CodecOutputSample>::stop()
{
    microphone_.stop_recording();
}

template <typename MicrophoneSample, typename CodecOutputSample>
CyclicCallStatus AudioProcessor<MicrophoneSample, CodecOutputSample>::cyclic()
{
    if (is_data_frame_pending_)
    {
        is_data_frame_pending_ = false;
        const auto result = microphone_.get_samples(sample_);
        if (result::Result::OK != result)
        {
            // TODO: define an appropriate action
            // - early option: propagate the data to the application for the microphone operation check
            // - the good option: run the data through the codec and then push the compressed data to the application
            return CyclicCallStatus::ERROR;
        }
        const auto processing_result = codec_.encode(sample_, processed_sample_);
        if (result::Result::OK != processing_result)
        {
            return CyclicCallStatus::ERROR;
        }
        return CyclicCallStatus::DATA_READY;
    }

    return CyclicCallStatus::NO_ACTION;
}

template <typename MicrophoneSample, typename CodecOutputSample>
void AudioProcessor<MicrophoneSample, CodecOutputSample>::microphone_data_ready_callback()
{
    is_data_frame_pending_ = true;
}

}
