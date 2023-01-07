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
    microphone_.register_data_ready_callback(std::bind(&AudioProcessor<MicrophoneSample>::microphone_data_ready_callback, this));
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

    NRF_LOG_INFO("pdm: interrupt was called %d times", pdm_interrupt_calls_);
}

template <typename MicrophoneSample>
void AudioProcessor<MicrophoneSample>::microphone_data_ready_callback()
{
    // TODO: signal the class user that data is ready for use. 
    pdm_interrupt_calls_++;
}

}
