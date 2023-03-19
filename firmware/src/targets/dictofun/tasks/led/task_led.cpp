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

#include "nrf_drv_pwm.h"

namespace led
{
constexpr uint16_t RED_LED_PIN = LED_3;
constexpr uint16_t BLUE_LED_PIN = LED_1;
constexpr uint16_t GREEN_LED_PIN = LED_2;

constexpr uint32_t task_led_queue_wait_time{10};
constexpr uint32_t slow_blinking_period{2000};
constexpr uint32_t fast_blinking_period{600};

// static nrf_drv_pwm_t m_pwm0 = NRF_DRV_PWM_INSTANCE(0);

void task_led(void * context_ptr)
{
    Context& context{*(reinterpret_cast<Context*>(context_ptr))};

    CommandQueueElement command_buffer;

    while(1)
    {
        const auto queue_receive_status = xQueueReceive(
            context.commands_queue, 
            reinterpret_cast<void *>(&command_buffer), 
            task_led_queue_wait_time
        );
        if (pdPASS == queue_receive_status)
        {
            NRF_LOG_INFO("led: received command [%d-%d]", command_buffer.color, command_buffer.state);
        }
    }
}

}
