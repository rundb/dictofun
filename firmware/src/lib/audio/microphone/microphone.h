// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#pragma once

#include "result.h"
#include <functional>

namespace audio
{

/// @brief Microphone class abstracts implementation of a particular microphone media. 
/// Microphone works in the following way:
template<typename SampleType>
class Microphone
{
public:
    virtual void init() = 0;
    virtual void start_recording() = 0;
    virtual void stop_recording() = 0;

    /// @brief This method should provide the pointer to the SamplesType structure containing
    /// last acquired audio sample.
    /// @param sample - pointer to the data structure where the last collected samples should  be placed
    /// @return OK, if correct samples have been provided, Error code otherwise.
    /// @note data should be acquired within a certain time limit after the data_ready callback. If delay
    /// is too long, the microphone class is allowed to invalidate the data, error can be returned in this case. 
    virtual result::Result get_samples(SampleType& sample) = 0;
    
    /// TODO: replace with a callable (check if std::function<..> is available for this purpose).
    using DataReadyCallback = std::function<void()>;

    /// @brief Pass a callback that should be executed in the interrupt upon the sample's readiness. 
    /// @param callback pointer to a function that signals the user about the data readiness. 
    virtual void register_data_ready_callback(DataReadyCallback callback) = 0;
};

}
