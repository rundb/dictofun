// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */
#pragma once

namespace audio
{

template <typename MicrophoneSample>
void AudioProcessor<MicrophoneSample>::init()
{
    microphone_.init();
    microphone_.register_data_ready_callback(
        std::bind(&AudioProcessor<MicrophoneSample>::microphone_data_ready_callback, this));
}

template <typename MicrophoneSample>
void AudioProcessor<MicrophoneSample>::start()
{
    microphone_.start_recording();
}

template <typename MicrophoneSample>
void AudioProcessor<MicrophoneSample>::stop()
{
    microphone_.stop_recording();
}

template <typename MicrophoneSample>
void AudioProcessor<MicrophoneSample>::cyclic()
{
    if (is_data_frame_pending_)
    {
        is_data_frame_pending_ = false;
        const auto result = microphone_.get_samples(sample_);
        if (result::Result::OK != result)
        {
            // TODO: define an appropriate action
            return;
        }
    }
}

template <typename MicrophoneSample>
void AudioProcessor<MicrophoneSample>::microphone_data_ready_callback()
{
    is_data_frame_pending_ = true;
}

}
