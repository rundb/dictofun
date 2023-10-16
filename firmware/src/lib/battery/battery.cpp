// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */
#include "battery.h"

namespace battery
{

BatteryMeasurement::BatteryMeasurement(adc::AdcIf& adc,
                                       const adc::AdcIf::AnalogInputId battery_pin_id,
                                       const float multiplex_factor)
    : _adc(adc)
    , _battery_pin_id(battery_pin_id)
    , _multiplex_factor(multiplex_factor)
{ }

result::Result BatteryMeasurement::init()
{
    for (auto i = 0; i < AVERAGING_SAMPLING_COUNT; ++i)
    {
        [[maybe_unused]] auto level = get_battery_level();
    }
    return result::Result::OK;
}

BatteryMeasurement::BatteryVoltage BatteryMeasurement::get_battery_voltage()
{
    adc::AdcIf::Measurement measurement{0.0};

    const auto measurement_result = _adc.get_measurement(_battery_pin_id, measurement);
    if(result::Result::OK != measurement_result)
    {
        return 0.0;
    }
    return measurement * _multiplex_factor;
}

BatteryMeasurement::BatteryLevel BatteryMeasurement::get_battery_level()
{
    const auto battery_voltage = get_battery_voltage();
    _battery_level_buffers[_battery_lvl_idx] = battery_voltage;
    _battery_lvl_idx = (_battery_lvl_idx + 1) % AVERAGING_SAMPLING_COUNT;
    const auto avg_batt_voltage{get_average_voltage()};
    return convert_battery_voltage_to_level(avg_batt_voltage);
}

BatteryMeasurement::BatteryVoltage BatteryMeasurement::get_average_voltage()
{
    float accumulator{0};
    for(auto& m : _battery_level_buffers)
    {
        accumulator += m;
    }
    return accumulator / AVERAGING_SAMPLING_COUNT;
}

uint8_t BatteryMeasurement::convert_battery_voltage_to_level(float voltage)
{
    uint32_t idx{0};
    for(auto i = 0; i < REFERENCE_VOLTAGES_COUNT; ++i)
    {
        if(voltage < REFERENCE_VOLTAGES[i])
        {
            break;
        }
        ++idx;
    }
    return MAX_BATTERY_LEVEL / REFERENCE_VOLTAGES_COUNT * idx;
}

} // namespace battery
