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
constexpr uint16_t UNCONNECTED_PIN = 30;

constexpr uint32_t task_led_queue_wait_time{10};

static constexpr uint16_t pwm_top_value = 10000;

nrf_drv_pwm_config_t const pwm0_config = 
{
    .output_pins =
    {
        RED_LED_PIN | NRF_DRV_PWM_PIN_INVERTED,
        GREEN_LED_PIN | NRF_DRV_PWM_PIN_INVERTED,
        BLUE_LED_PIN | NRF_DRV_PWM_PIN_INVERTED,
        UNCONNECTED_PIN | NRF_DRV_PWM_PIN_INVERTED,
    },
    .irq_priority = APP_IRQ_PRIORITY_LOWEST,
    .base_clock   = NRF_PWM_CLK_1MHz,
    .count_mode   = NRF_PWM_MODE_UP,
    .top_value    = pwm_top_value,
    .load_mode    = NRF_PWM_LOAD_INDIVIDUAL,
    .step_mode    = NRF_PWM_STEP_AUTO
};

static nrf_drv_pwm_t m_pwm0 = NRF_DRV_PWM_INSTANCE(0);

static void pwm0_handler(nrf_drv_pwm_evt_type_t event_type) { }

// This code implements N levels of LED brightness (starting with 4)
static constexpr uint16_t led_duty_cycles[] = {
    0,
    static_cast<uint16_t>(pwm_top_value * 0.05),
    static_cast<uint16_t>(pwm_top_value * 0.08),
    static_cast<uint16_t>(pwm_top_value * 0.15),
    static_cast<uint16_t>(pwm_top_value * 0.3),
    static_cast<uint16_t>(pwm_top_value * 0.35),
    static_cast<uint16_t>(pwm_top_value * 0.3),
    static_cast<uint16_t>(pwm_top_value * 0.15),
    static_cast<uint16_t>(pwm_top_value * 0.08),
    static_cast<uint16_t>(pwm_top_value * 0.05),
    0,
};
static constexpr uint32_t duty_cycles_count{sizeof(led_duty_cycles) / sizeof(led_duty_cycles[0])};
static constexpr uint16_t max_duty_cycle{led_duty_cycles[duty_cycles_count / 2]};

bool is_red_active(const Color color)
{
    return Color::RED == color || Color::PURPLE == color || Color::YELLOW == color || Color::WHITE == color;
}

bool is_green_active(const Color color)
{
    return Color::GREEN == color || Color::BRIGHT_BLUE == color || Color::YELLOW == color || Color::WHITE == color;
}

bool is_blue_active(const Color color)
{
    return Color::DARK_BLUE == color || Color::BRIGHT_BLUE == color || Color::PURPLE == color || Color::WHITE == color;
}


void task_led(void * context_ptr)
{
    Context& context{*(reinterpret_cast<Context*>(context_ptr))};

    CommandQueueElement command_buffer;

    Color prev_color{Color::RED};
    Color active_color{Color::RED};
    State prev_state{State::OFF};
    State active_state{State::OFF};

    const auto nrfx_init_result = nrfx_pwm_init(&m_pwm0, &pwm0_config, pwm0_handler);
    if (NRFX_SUCCESS != nrfx_init_result)
    {
        NRF_LOG_WARNING("led: error at PWM init call (%d)", nrfx_init_result);
    }
    
    nrf_pwm_values_individual_t seq_values{0, 0, 0, 0};
    
    nrf_pwm_sequence_t seq{0};
    seq.values.p_individual = &seq_values;
    seq.length = NRF_PWM_VALUES_LENGTH(seq_values);

    nrfx_pwm_simple_playback(&m_pwm0, &seq, 1, NRFX_PWM_FLAG_LOOP);

    static constexpr uint32_t slow_update_period{2000};
    static constexpr uint32_t slow_pwm_change_period{slow_update_period / duty_cycles_count};
    static constexpr uint32_t fast_update_period{500};
    static constexpr uint32_t fast_pwm_change_period{fast_update_period / duty_cycles_count};

    // since blue LED is known to be less bright, it's duty cycle should be multiplied (generally there should be a 
    // calculated table of optimal intensities of all colors for all brightness levels)
    static constexpr double blue_dc_multiplier{1.8};

    uint32_t last_update_tick{0};
    while(1)
    {
        // p.1: check out any incoming messages
        const auto queue_receive_status = xQueueReceive(
            context.commands_queue, 
            reinterpret_cast<void *>(&command_buffer), 
            task_led_queue_wait_time
        );
        if (pdPASS == queue_receive_status)
        {
            NRF_LOG_INFO("led: received command [%d-%d]", command_buffer.color, command_buffer.state);
            active_color = command_buffer.color;
            active_state = command_buffer.state;
        }
        
        // p.2: recalculate PWM duty cycles
        const auto tick = xTaskGetTickCount();

        if (prev_state != active_state && active_state == State::OFF)
        {
            NRF_LOG_INFO("detected off request");
            nrfx_pwm_stop(&m_pwm0, false);
        }
        if (active_state == State::SLOW_GLOW || active_state == State::FAST_GLOW)
        {
            const auto update_period = (active_state == State::SLOW_GLOW) ? slow_update_period : fast_update_period;
            const auto pwm_change_period = (active_state == State::SLOW_GLOW) ? slow_pwm_change_period : fast_pwm_change_period;
            if ((tick - last_update_tick) > pwm_change_period)
            {
                const uint32_t pwm_values_index{((tick % update_period) / pwm_change_period) % duty_cycles_count};

                // configure red channel
                seq_values.channel_0 = (is_red_active(active_color)) ? led_duty_cycles[pwm_values_index] : 0;
                // configure blue channel
                seq_values.channel_1 = (is_green_active(active_color)) ? led_duty_cycles[pwm_values_index] : 0;
                seq_values.channel_1 *= blue_dc_multiplier;
                // configure green channel
                seq_values.channel_2 = (is_blue_active(active_color)) ? led_duty_cycles[pwm_values_index] : 0;
                nrfx_pwm_stop(&m_pwm0, true);
                nrfx_pwm_simple_playback(&m_pwm0, &seq, 1, NRFX_PWM_FLAG_LOOP);

                last_update_tick = tick;
            }
        }
        if (
            (prev_state != active_state && active_state == State::ON) ||
            (prev_color != active_color && active_state == State::ON)
           )
        {
            NRF_LOG_INFO("detected on request");
            // configure red channel
            seq_values.channel_0 = (is_red_active(active_color)) ? max_duty_cycle : 0;
            // configure blue channel
            seq_values.channel_1 = (is_green_active(active_color)) ? max_duty_cycle : 0;
            seq_values.channel_1 *= blue_dc_multiplier;
            // configure green channel
            seq_values.channel_2 = (is_blue_active(active_color)) ? max_duty_cycle : 0;
            nrfx_pwm_stop(&m_pwm0, true);
            nrfx_pwm_simple_playback(&m_pwm0, &seq, 1, NRFX_PWM_FLAG_LOOP);
        }

        prev_state = active_state;
        prev_color = active_color;
    }
}

}
