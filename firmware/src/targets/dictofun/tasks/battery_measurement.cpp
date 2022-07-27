// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#include "tasks/battery_measurement.h"
#include <nrfx_saadc.h>

namespace battery
{

static void battery_adc_event_handler(nrfx_saadc_evt_t const * p_event)
{
    BatteryMeasurement::getInstance().isr();
}

BatteryMeasurement * BatteryMeasurement::_instance{nullptr};

void BatteryMeasurement::init()
{
    if (_isInitialized)
    {
        // TODO: assert
        return;
    }
    _isInitialized = true;
    auto err_code = nrfx_saadc_init(&_adc_config, battery_adc_event_handler);
    APP_ERROR_CHECK(err_code);  
    err_code = nrfx_saadc_channel_init(_analog_input_id, &_adc_channel_config);
    APP_ERROR_CHECK(err_code);
}

void BatteryMeasurement::start()
{
  _samples_gathered = 0;
  auto err_code = nrfx_saadc_buffer_convert(_samples, 1);
  APP_ERROR_CHECK(err_code);
  err_code = nrfx_saadc_sample();
  APP_ERROR_CHECK(err_code);
  _isBusy = true;
}

bool BatteryMeasurement::isBusy()
{
    // if (nrfx_adc_is_busy())
    // {
    //     return true;
    // }
    return _isBusy;
}

void BatteryMeasurement::isr()
{
    if (_samples_gathered < SAMPLES_COUNT)
    {
        ++_samples_gathered;
        nrfx_saadc_buffer_convert(&_samples[_samples_gathered], 1);
        const auto err_code = nrfx_saadc_sample();
        APP_ERROR_CHECK(err_code);
    }
    else
    {
        _isBusy = false;
        _battery_voltage = 0.0;
        for (auto in: _samples)
        {
            _battery_voltage += static_cast<float>(in);
        }
        _battery_voltage /= SAMPLES_COUNT;
        _battery_voltage = 3.3 * 2.0 * _battery_voltage / 1024.0;
    }
}

float BatteryMeasurement::voltage()
{
    return _battery_voltage;
}

uint8_t BatteryMeasurement::level()
{
    const float base = 2.9;
    if (_battery_voltage < base)
    {
        return 0;
    }
    const float max_level = 4.0;
    if (_battery_voltage > max_level)
    {
      return 100;
    }
    float level = (_battery_voltage - base) / (max_level - base);
    return static_cast<uint8_t>(level * 100);
}


}