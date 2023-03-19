// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "task_led.h"
#include <stdint.h>
#include "nrf_gpio.h"
#include "boards.h"
#include "nrf_log.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

namespace led
{
constexpr uint16_t RED_LED_PIN = LED_3;
constexpr uint16_t BLUE_LED_PIN = LED_1;
constexpr uint16_t GREEN_LED_PIN = LED_2;

constexpr uint32_t task_led_queue_wait_time{10};
constexpr uint32_t slow_blinking_period{2000};
constexpr uint32_t fast_blinking_period{600};

// constexpr auto colors_count{static_cast<int>(Color::COUNT)};

// const uint16_t indexes[colors_count] {RED_LED_PIN, BLUE_LED_PIN, GREEN_LED_PIN};
// State states[colors_count] {State::OFF, State::OFF, State::OFF};

CommandQueueElement command_buffer;

void task_led(void * context_ptr)
{
    Context& context{*(reinterpret_cast<Context*>(context_ptr))};
    // states[0] = State::OFF;
    // states[1] = State::OFF;
    // states[2] = State::OFF;

    // nrf_gpio_cfg_output(RED_LED_PIN);
    // nrf_gpio_cfg_output(BLUE_LED_PIN);
    // nrf_gpio_cfg_output(GREEN_LED_PIN);

    // nrf_gpio_pin_write(RED_LED_PIN, LED_OFF);
    // nrf_gpio_pin_write(BLUE_LED_PIN, LED_OFF);
    // nrf_gpio_pin_write(GREEN_LED_PIN, LED_OFF);

    // NRF_LOG_INFO("task led: initialized");

    while(1)
    {
        const auto queue_receive_status = xQueueReceive(
            context.commands_queue, 
            reinterpret_cast<void *>(&command_buffer), 
            task_led_queue_wait_time
        );
        if (pdPASS == queue_receive_status)
        {
            NRF_LOG_INFO("led: received command");
            // states[static_cast<int>(command_buffer.color)] = command_buffer.state;
        }
        // for (auto i = 0; i < colors_count; ++i)
        // {
        //     nrf_gpio_pin_write(indexes[i], (states[i] == State::OFF) ? LED_OFF : LED_ON);
        // }
    }
}

}
