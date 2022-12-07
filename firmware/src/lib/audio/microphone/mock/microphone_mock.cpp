#include "microphone_mock.h"

namespace audio
{
    
void MicrophoneMock::start_recording()
{
    
}

void MicrophoneMock::stop_recording()
{

}

result::Result MicrophoneMock::get_samples(MockAudioSample& samples)
{
    return result::Result::ERROR_NOT_IMPLEMENTED;
}

void MicrophoneMock::register_data_ready_callback(DataReadyCallback callback)
{

}

}