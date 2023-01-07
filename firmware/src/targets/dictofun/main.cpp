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
#include "queue.h"

// TODO: this dependency here is really bad
#include "microphone_pdm.h"

#include <task_state.h>
#include "task_audio.h"
#include "task_audio_tester.h"
#include "task_cli_logger.h"

#include <stdint.h>

#include "freertos_wrappers.hpp"

// clang-format off
// ============================= Tasks ======================================

application::TaskDescriptor<256, 1> audio_task;
application::TaskDescriptor<256, 1> audio_tester_task;
application::TaskDescriptor<256, 1> log_task;
application::TaskDescriptor<256, 2> systemstate_task;

// ============================= Queues =====================================

application::QueueDescriptor<logger::CliCommandQueueElement, 1>  cli_commands_queue;
application::QueueDescriptor<logger::CliStatusQueueElement, 1>   cli_status_queue; // This thing is under a big doubt, I don't think it's needed
application::QueueDescriptor<audio::CommandQueueElement, 1>      audio_commands_queue;
application::QueueDescriptor<audio::StatusQueueElement, 1>       audio_status_queue;

application::QueueDescriptor<audio::microphone::PdmMicrophone<audio::pdm_sample_size>::SampleType, 3>          audio_data_queue;

// ============================= Timers =====================================

StaticTimer_t record_timer_buffer;
TimerHandle_t record_timer_handle{nullptr};

// ============================ Contexts ====================================
logger::CliContext      cli_context;
systemstate::Context    systemstate_context;
audio::Context          audio_context;
audio::tester::Context  audio_tester_context;

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

    // Queues' initialization
    const auto cli_commands_init_result = cli_commands_queue.init();
    if (result::Result::OK != cli_commands_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    const auto cli_status_init_result = cli_status_queue.init();
    if (result::Result::OK != cli_status_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    const auto audio_commands_init_result = audio_commands_queue.init();
    if (result::Result::OK != audio_commands_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    const auto audio_data_init_result = audio_data_queue.init();
    if (result::Result::OK != audio_data_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    const auto audio_status_init_result = audio_status_queue.init();
    if (result::Result::OK != audio_status_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    // Timers' initialization
    record_timer_handle = xTimerCreateStatic("AUDIO", 1, pdFALSE, nullptr, systemstate::record_end_callback, &record_timer_buffer);
    if (nullptr == record_timer_handle)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    // Tasks' initialization
    audio_context.commands_queue = audio_commands_queue.handle;
    audio_context.status_queue = audio_status_queue.handle;
    audio_context.data_queue = audio_data_queue.handle;

    const auto audio_task_init_result = audio_task.init(audio::task_audio, "AUDIO", &audio_context);
    if (result::Result::OK != audio_task_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    audio_tester_context.data_queue = audio_data_queue.handle;

    const auto audio_tester_task_init_result = audio_tester_task.init(
        audio::tester::task_audio_tester, 
        "AUTEST", 
        &audio_tester_context);
    if (result::Result::OK != audio_tester_task_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    cli_context.cli_commands_handle = cli_commands_queue.handle;
    cli_context.cli_status_handle = cli_status_queue.handle;

    const auto log_task_init_result = log_task.init(logger::task_cli_logger, "CLI", &cli_context);
    if (result::Result::OK != log_task_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    systemstate_context.cli_commands_handle = cli_commands_queue.handle;
    systemstate_context.cli_status_handle = cli_status_queue.handle;
    systemstate_context.audio_commands_handle = audio_commands_queue.handle;
    systemstate_context.audio_status_handle = audio_status_queue.handle;
    systemstate_context.record_timer_handle = record_timer_handle;

    const auto systemstate_task_init_result = systemstate_task.init(
        systemstate::task_system_state, 
        "STATE", 
        &systemstate_context);
    if (result::Result::OK != systemstate_task_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    vTaskStartScheduler();

}
