// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#pragma once

#include <nrfx_saadc.h>

namespace battery
{

class BatteryMeasurement
{
public:
    explicit BatteryMeasurement(const uint8_t analog_input_id)
        : _analog_input_id(analog_input_id)
        , _adc_config(NRFX_SAADC_DEFAULT_CONFIG)
        , _adc_channel_config(NRFX_SAADC_DEFAULT_CHANNEL_CONFIG_SE(_analog_input_id))
    {
        if(_instance != nullptr)
        {
            // assert
            while(1)
                ;
        }
        _instance = this;
    }
    void init();
    inline bool isInitialized()
    {
        return _isInitialized;
    }
    void start();
    bool isBusy();
    float voltage();
    uint8_t level();
    void isr();
    static inline BatteryMeasurement& getInstance()
    {
        return *_instance;
    }
    static constexpr float MINIMAL_OPERATIONAL_VOLTAGE{3.2};

private:
    static BatteryMeasurement* _instance;
    bool _isInitialized{false};
    uint8_t _analog_input_id;
    bool _isBusy{false};
    nrfx_saadc_config_t _adc_config;
    nrf_saadc_channel_config_t _adc_channel_config;
    static constexpr size_t SAMPLES_COUNT{10};
    size_t _samples_gathered{0};
    int16_t _samples[SAMPLES_COUNT];
    float _battery_voltage{0.0};
};

} // namespace battery
