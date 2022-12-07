#pragma once

namespace audio
{
namespace microphone
{
    
template <size_t SampleBufferSize>
void PdmMicrophone<SampleBufferSize>::start_recording() 
{
    nrf_drv_pdm_start();
}

template <size_t SampleBufferSize>
void PdmMicrophone<SampleBufferSize>::stop_recording() 
{

}

template <size_t SampleBufferSize>
result::Result PdmMicrophone<SampleBufferSize>::get_samples(SampleType& samples) 
{
    return result::Result::ERROR_NOT_IMPLEMENTED;
}

template <size_t SampleBufferSize>
void PdmMicrophone<SampleBufferSize>::register_data_ready_callback(PdmDataReadyCallback callback)
{

}


}
}