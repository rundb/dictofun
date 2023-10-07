// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */
#include "task_battery.h"
#include "adc.h"
#include "battery.h"
#include "nrf_log.h"

#include "FreeRTOS.h"
#include "task.h"

namespace battery
{

static constexpr adc::AdcIf::AnalogInputId battery_pin_id{2};
static constexpr float battery_conversion_factor{2.0};
static const adc::NrfAdc::Config nrf_adc_config{1, {battery_pin_id, 0, 0, 0}};

Context* context{nullptr};

struct BatteryMeasurementObjects
{
    explicit BatteryMeasurementObjects(const adc::AdcIf::PinId pin_id,
                                       const float battery_conversion_factor)
        : nrf_adc(nrf_adc_config)
        , battery_measurement(nrf_adc, pin_id, battery_conversion_factor)
    { }
    adc::NrfAdc nrf_adc;
    battery::BatteryMeasurement battery_measurement;
};

static BatteryMeasurementObjects _battery_measurement_objects{battery_pin_id,
                                                              battery_conversion_factor};

void task_battery(void* context_ptr)
{
    NRF_LOG_INFO("task batt: initialized")
    auto& adc{_battery_measurement_objects.nrf_adc};
    auto& battery{_battery_measurement_objects.battery_measurement};

    context = reinterpret_cast<Context*>(context_ptr);

    uint32_t iterations_counter{0};
    constexpr uint32_t voltage_printout_period{20};
    const auto nrf_init_result = adc.init();
    if(result::Result::OK != nrf_init_result)
    {
        NRF_LOG_ERROR("nrf adc: failed to initialize");
    }
    const auto battery_measurement_init_result = battery.init();
    if(result::Result::OK != battery_measurement_init_result)
    {
        NRF_LOG_ERROR("battery: failed to initialize");
    }
    while(1)
    {
        vTaskDelay(100);
        const auto battery_level = battery.get_battery_level();
        const auto voltage = battery.get_average_voltage();

        battery::MeasurementsQueueElement measurement;
        measurement.battery_voltage_level = voltage;
        measurement.battery_percentage = battery_level;

        const auto send_result = xQueueSend(context->measurements_handle, &measurement, 0);
        if(pdTRUE != send_result)
        {
            NRF_LOG_ERROR("failed to send battery measurement");
        }
    }
}

} // namespace battery
