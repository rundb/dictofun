#pragma once

namespace audio
{

template <typename MicrophoneSample>
void AudioProcessor<MicrophoneSample>::start()
{
    microphone_.start_recording();
}

}