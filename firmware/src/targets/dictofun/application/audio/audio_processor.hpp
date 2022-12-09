// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */
#pragma once

namespace audio
{

template <typename MicrophoneSample>
void AudioProcessor<MicrophoneSample>::start()
{
    microphone_.start_recording();
}

}
