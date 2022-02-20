/*
 * Copyright (c) 2021 Roman Turkin 
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "tasks/task_led.h"
#include <stdint.h>
#include <nrf_gpio.h>
#include <libraries/timer/app_timer.h>
#include "boards/boards.h"

namespace led
{
static const uint16_t RED_LED_PIN = LED_3;
static const uint16_t BLUE_LED_PIN = LED_1;
static const uint16_t GREEN_LED_PIN = LED_2;

LedState colors[COLORS_COUNT] {OFF, OFF, OFF};

IndicationState _current_state{PREPARING};

void task_led_init()
{
    nrf_gpio_pin_set(RED_LED_PIN);
    nrf_gpio_pin_set(BLUE_LED_PIN);
    nrf_gpio_pin_set(GREEN_LED_PIN);

    nrf_gpio_cfg_output(RED_LED_PIN);
    nrf_gpio_cfg_output(BLUE_LED_PIN);
    nrf_gpio_cfg_output(GREEN_LED_PIN);
}

static const uint32_t SHORT_BLINKING_PERIOD = 100;
static const uint32_t LONG_BLINKING_PERIOD = 10 * SHORT_BLINKING_PERIOD;
void task_led_cyclic()
{
    const auto timestamp = app_timer_cnt_get();
    const auto is_slow_blinking_on = ((timestamp / LONG_BLINKING_PERIOD) & 0x01) > 0;
    const auto is_fast_blinking_on = ((timestamp / SHORT_BLINKING_PERIOD) & 0x01) > 0;
    for (int i = 0; i < COLORS_COUNT; ++i)
    {
        uint16_t index = (i == RED) ? RED_LED_PIN : (i == GREEN) ? GREEN_LED_PIN : BLUE_LED_PIN;
        switch (colors[i])
        {
            case OFF:
            {
                nrf_gpio_pin_set(index);
                break;
            }
            case ON:
            {
                nrf_gpio_pin_clear(index);
                break;
            }
            case SLOW_BLINKING:
            {
                if (is_slow_blinking_on)
                {
                    nrf_gpio_pin_clear(index);
                }
                else
                {
                    nrf_gpio_pin_set(index);
                }
                break;
            }
            case FAST_BLINKING:
            {
                if (is_fast_blinking_on)
                {
                    nrf_gpio_pin_clear(index);
                }
                else
                {
                    nrf_gpio_pin_set(index);
                }
                break;
            }
        }
    }
}


// PREPARING -> green slow blinking
// RECORDING -> RED fast blinking
// CONNECTING -> BLUE slow blinking
// SENDING -> Blue fast blinking
// SHUTTING_DOWN -> green fast blinking

void task_led_set_indication_state(IndicationState state)
{
    _current_state = state;
    switch (_current_state)
    {
        case PREPARING:
        {
            colors[RED] = OFF;
            colors[GREEN] = SLOW_BLINKING;
            colors[BLUE] = OFF;
            break;
        }
        case RECORDING:
        {
            colors[RED] = FAST_BLINKING;
            colors[GREEN] = OFF;
            colors[BLUE] = OFF;
            break;
        }
        case CONNECTING:
        {
            colors[RED] = OFF;
            colors[GREEN] = OFF;
            colors[BLUE] = SLOW_BLINKING;
            break;
        }
        case SENDING:
        {
            colors[RED] = OFF;
            colors[GREEN] = OFF;
            colors[BLUE] = FAST_BLINKING;
            break;
        }
        case SHUTTING_DOWN:
        {
            colors[RED] = OFF;
            colors[GREEN] = FAST_BLINKING;
            colors[BLUE] = OFF;
            break;
        }
        case INDICATION_OFF:
        {
            colors[RED] = SLOW_BLINKING;
            colors[GREEN] = SLOW_BLINKING;
            colors[BLUE] = SLOW_BLINKING;
        }
    }
}

}