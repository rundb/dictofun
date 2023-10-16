// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#pragma once

#include "adc_if.h"
#include "result.h"

namespace battery
{

/// @brief This class implements basic battery measurement
/// TODO: there has to be a synchronization method allowing to estimate the load on the battery at the moment of measurement
class BatteryMeasurement
{
public:
    // TODO: consider making this value wrapped into optional
    using BatteryVoltage = float;
    using BatteryLevel = uint8_t;

    explicit BatteryMeasurement(adc::AdcIf& adc,
                                const adc::AdcIf::PinId battery_pin_id,
                                const float multiplex_factor);

    result::Result init();

    BatteryLevel get_battery_level();
    BatteryVoltage get_average_voltage();

private:
    adc::AdcIf& _adc;
    const adc::AdcIf::PinId _battery_pin_id;
    const float _multiplex_factor;

    static constexpr uint32_t AVERAGING_SAMPLING_COUNT{4};
    float _battery_level_buffers[AVERAGING_SAMPLING_COUNT]{0.0};
    uint32_t _battery_lvl_idx{0};
    BatteryVoltage get_battery_voltage();

    uint8_t convert_battery_voltage_to_level(float voltage);

    static constexpr uint32_t REFERENCE_VOLTAGES_COUNT{8};
    const float REFERENCE_VOLTAGES[REFERENCE_VOLTAGES_COUNT]{
        3.1,
        3.2,
        3.25,
        3.3,
        3.35,
        3.4,
        3.45,
        3.5,
    };

    static constexpr uint32_t MAX_BATTERY_LEVEL{100};
};
} // namespace battery
