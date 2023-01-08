// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */
#include "task_audio_tester.h"
#include "nrf_log.h"

namespace audio
{
namespace tester
{

float calculate_average(uint8_t * buffer, size_t size)
{
    int sum{0};
    for (size_t i = 0; i < size; ++i)
    {
        sum += static_cast<int>(buffer[i]);
    }
    return sum / size;
}

float calculate_deviation(float average, uint8_t * buffer, size_t size)
{
    float deviation_accum{0.0};
    for (size_t i = 0; i < size; ++i)
    {
        float v = static_cast<float>(buffer[i]);
        deviation_accum += (v - average) * (v - average);
    }
    return deviation_accum / (size - 1);
}

void task_audio_tester(void * context_ptr)
{
    Context& context{*(reinterpret_cast<Context *>(context_ptr))};
    constexpr size_t buffer_size{128}; // TODO: derive this size from the application configuration
    uint8_t buffer[buffer_size]{0};
    ControlQueueElement command;

    size_t received_samples_count{0};
    float last_sample_average{0.0};
    float last_sample_deviation{0.0};
    bool is_tester_active{false};
    while(1)
    {
        if (is_tester_active)
        {
            const auto data_result = xQueueReceive(context.data_queue, reinterpret_cast<void *>(buffer), 10);
            if (pdPASS == data_result)
            {
                received_samples_count++;
                last_sample_average = calculate_average(buffer, buffer_size);
                last_sample_deviation = calculate_deviation(last_sample_average, buffer, buffer_size);
            }
        }

        const auto commands_result = xQueueReceive(context.commands_queue, reinterpret_cast<void *>(&command), 1);
        if (pdPASS == commands_result)
        {
            is_tester_active = command.should_enable_tester;
            if (!command.should_enable_tester)
            {
                NRF_LOG_INFO("autest: received %d samples, E=%d, sigma^2=%d", 
                    received_samples_count, 
                    static_cast<int>(last_sample_average),
                    static_cast<int>(last_sample_deviation)
                );
            }
        }
    }
}
}
}
