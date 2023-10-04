// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "adc.h"
#include <nrfx_saadc.h>
#include <algorithm>
#include <cstring>

namespace adc
{

NrfAdc * NrfAdc::_instance{nullptr};

NrfAdc::NrfAdc(const Config& config)
: _config(config)
{
    if (_instance != nullptr)
    {
        // TODO: implement a proper assert
    }

    _instance = this;
    // TODO: add an assertion on the invalid ADC config (count >= max_count || count == 0)
    // if (config.count > MAX_ADC_PINS_COUNT || config.count == 0)
    // {
    //      ASSERT();
    // }
}

static void adc_event_handler(nrfx_saadc_evt_t const* p_event);

result::Result NrfAdc::init()
{
    nrfx_saadc_config_t _adc_config = NRFX_SAADC_DEFAULT_CONFIG;
    _adc_config.resolution = NRF_SAADC_RESOLUTION_12BIT; // 12-bit

    const auto init_result = nrfx_saadc_init(&_adc_config, adc_event_handler);
    if (init_result != NRFX_SUCCESS)
    {
        return result::Result::ERROR_GENERAL;
    }
    
    for (auto i = 0; i < _config.count; ++i)
    {
        const auto analog_input_id = _config.analog_input_ids[i];
        nrf_saadc_channel_config_t _adc_channel_config = NRFX_SAADC_DEFAULT_CHANNEL_CONFIG_SE(analog_input_id);
        const auto channel_init_result = nrfx_saadc_channel_init(analog_input_id, &_adc_channel_config);
        if (NRFX_SUCCESS != channel_init_result)
        {
            return result::Result::ERROR_GENERAL;
        }
    }
    
    return result::Result::OK;
}

result::Result NrfAdc::get_measurement(const AnalogInputId pin_id, Measurement& measurement)
{
    int16_t measurement_i16{0};
    const auto sample_result = nrfx_saadc_sample_convert(_config.analog_input_ids[0], &measurement_i16);
    if (NRFX_SUCCESS != sample_result)
    {
        return result::Result::ERROR_GENERAL;
    }
    measurement = static_cast<float>(measurement_i16) * ADC_BASE_VOLTAGE / static_cast<float>(ADC_12_BIT_MAX_VALUE);

    return result::Result::OK;
}

void adc_event_handler(nrfx_saadc_evt_t const* p_event)
{
    if (p_event == nullptr) return;
    switch (p_event->type)
    {
        case NRFX_SAADC_EVT_DONE:
        {
            NrfAdc::instance().conversion_completed(p_event->data.done.size, p_event->data.done.p_buffer);
            break;
        }
        case NRFX_SAADC_EVT_LIMIT:
        {
            break;
        }
        case NRFX_SAADC_EVT_CALIBRATEDONE:
        {
            break;
        }
        default:
        {
            // TODO: add assertion
        }
    }
}

void NrfAdc::conversion_completed(const uint16_t samples_count, int16_t * buffer)
{
    if (samples_count > 0 && buffer != nullptr)
    {
        memcpy(_measurements, buffer, std::min(samples_count, SAMPLES_BUFFER_SIZE));
        _is_measurement_pending = true;
    }
}

}
