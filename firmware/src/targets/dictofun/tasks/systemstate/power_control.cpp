// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "task_state.h"
#include "FreeRTOS.h"
#include "task.h"

#include "nrf_log.h"
#include "nrf_gpio.h"
#include "nrf_log_ctrl.h"

namespace systemstate
{

static constexpr uint8_t power_flipflop_clk{26};
static constexpr uint8_t power_flipflop_d{27};

void configure_power_latch()
{
    nrf_gpio_cfg_output(power_flipflop_clk);
    nrf_gpio_cfg_output(power_flipflop_d);

    nrf_gpio_pin_clear(power_flipflop_clk);
    nrf_gpio_pin_set(power_flipflop_d);

    vTaskDelay(1);
    nrf_gpio_pin_set(power_flipflop_clk);
    vTaskDelay(1);
    nrf_gpio_pin_clear(power_flipflop_clk);
    vTaskDelay(1);

}

void shutdown_ldo()
{
    NRF_LOG_INFO("shutting the system down");
    // Letting logger to print out all it needs to print
    NRF_LOG_FLUSH();
    vTaskDelay(100);
    nrf_gpio_pin_clear(power_flipflop_d);
    vTaskDelay(1);
    nrf_gpio_pin_clear(power_flipflop_clk);
    vTaskDelay(1);
    nrf_gpio_pin_set(power_flipflop_clk);
    vTaskDelay(1);
    nrf_gpio_pin_clear(power_flipflop_clk);
}

}