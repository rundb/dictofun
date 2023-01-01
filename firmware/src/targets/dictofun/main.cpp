// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#include "app_error.h"

#include <boards.h>
#include <nrf_gpio.h>
#include "nrf_drv_clock.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include <task_state.h>
#include "task_audio.h"
#include "task_cli_logger.h"

#include <stdint.h>

constexpr size_t audio_task_stack_size{256};
static StackType_t audio_task_stack[audio_task_stack_size] {0UL};
static StaticTask_t m_audio_task;
TaskHandle_t audio_task_handle{nullptr};

constexpr size_t log_task_stack_size{256};
static StackType_t log_task_stack[log_task_stack_size] {0UL};
static StaticTask_t m_log_task;
TaskHandle_t log_task_handle{nullptr};

void latch_ldo_enable()
{
    nrf_gpio_cfg_output(LDO_EN_PIN);
    nrf_gpio_cfg_input(BUTTON_PIN, NRF_GPIO_PIN_PULLDOWN);
    nrf_gpio_pin_set(LDO_EN_PIN);

    nrf_gpio_cfg(LDO_EN_PIN,
                 NRF_GPIO_PIN_DIR_OUTPUT,
                 NRF_GPIO_PIN_INPUT_DISCONNECT,
                 NRF_GPIO_PIN_PULLDOWN,
                 NRF_GPIO_PIN_H0S1,
                 NRF_GPIO_PIN_NOSENSE);
}

// TODO: reintegrate system elements after designing them in a test-driven way
int main()
{
    latch_ldo_enable();

    const auto err_code = nrf_drv_clock_init();
    APP_ERROR_CHECK(err_code);
    nrf_drv_clock_lfclk_request(NULL);

    logger::log_init();

    bsp_board_init(BSP_INIT_LEDS);

    audio_task_handle = xTaskCreateStatic(audio::task_audio, "AUDIO", 256, NULL, 1, audio_task_stack, &m_audio_task);
    if (nullptr == audio_task_handle)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    log_task_handle = xTaskCreateStatic(logger::task_cli_logger, "CLI", 256, NULL, 1, log_task_stack, &m_log_task);
    if (nullptr == log_task_handle)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    vTaskStartScheduler();

}
