// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */
#pragma once

#include "adc_if.h"
#include <stdint.h>
#include <utility>
#include <vector>

namespace adc 
{

class NrfAdc: public AdcIf 
{
public:
    struct Config
    {
        static constexpr uint32_t MAX_ADC_PINS_COUNT{4};
        uint8_t count{0};
        // TODO: replace with an appropriate container
        AnalogInputId analog_input_ids[MAX_ADC_PINS_COUNT]{0};
    };

    // TODO: provide NRF-specific configuration object
    explicit NrfAdc(const Config& config);
    result::Result init();
    result::Result get_measurement(const AnalogInputId analog_input_id, Measurement& measurement) override;

    static NrfAdc& instance() {return *_instance;}
    
    void conversion_completed(const uint16_t samples_count, int16_t * buffer);
    
    bool is_measurement_pending() { return _is_measurement_pending; }
    void clear_pending_flag() { _is_measurement_pending = false; }
private:
    static constexpr uint32_t ADC_10_BIT_MAX_VALUE{1023};
    static constexpr uint32_t ADC_12_BIT_MAX_VALUE{4095};
    static constexpr float ADC_BASE_VOLTAGE{3.3F};
    const Config _config;

    static NrfAdc * _instance;

    // Currently unused
    static constexpr uint16_t SAMPLES_BUFFER_SIZE{8};
    int16_t _measurements[SAMPLES_BUFFER_SIZE]{0};

    volatile bool _is_measurement_pending{false};
    
};

}
