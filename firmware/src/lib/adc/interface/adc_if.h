// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#pragma once

#include "result.h"
#include <stdint.h>

namespace adc 
{

// This class provides an abstract interface for ADC access and measurements acquiring.
class AdcIf
{
public:
    using PinId = uint8_t;
    using AnalogInputId = uint8_t;
    using Measurement = float;

    /// @brief This method retrieves collected measurement from the requested pin. 
    /// @param pin_id analog ID of the pin on which the measurements are taken
    /// @param measurement measured value
    /// @return OK, if the measurement has been acquired successfully, ERROR otherwise (i.e. if pin was not configured)
    virtual result::Result get_measurement(const AnalogInputId pin_id, Measurement& measurement) = 0;
};

} // namespace adc
