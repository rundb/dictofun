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

// clang-format off
constexpr size_t        audio_task_stack_size                   {256};
StackType_t             audio_task_stack[audio_task_stack_size] {0UL};
StaticTask_t            m_audio_task;
TaskHandle_t            audio_task_handle{nullptr};
UBaseType_t             audio_task_priority{1U};


constexpr size_t        log_task_stack_size{256};
StackType_t             log_task_stack[log_task_stack_size] {0UL};
StaticTask_t            m_log_task;
TaskHandle_t            log_task_handle{nullptr};
UBaseType_t             log_task_priority{1U};

constexpr size_t        system_state_task_stack_size{256};
StackType_t             system_state_task_stack[log_task_stack_size] {0UL};
StaticTask_t            system_state_task;
TaskHandle_t            system_state_task_handle{nullptr};
UBaseType_t             system_state_task_priority{2U};
// clang-format on

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

    audio_task_handle = xTaskCreateStatic(
        audio::task_audio,
        "AUDIO",
        audio_task_stack_size,
        NULL,
        audio_task_priority,
        audio_task_stack,
        &m_audio_task);
    if (nullptr == audio_task_handle)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    log_task_handle = xTaskCreateStatic(
        logger::task_cli_logger,
        "CLI",
        log_task_stack_size,
        NULL,
        log_task_priority,
        log_task_stack,
        &m_log_task);
    if (nullptr == log_task_handle)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    system_state_task_handle = xTaskCreateStatic(
        systemstate::task_system_state,
        "STATE",
        system_state_task_stack_size,
        NULL,
        system_state_task_priority,
        system_state_task_stack,
        &system_state_task);

    if (nullptr == system_state_task_handle)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    vTaskStartScheduler();

}
