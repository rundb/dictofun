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

#include <task_state.h>
#include "task_audio.h"
#include "task_cli_logger.h"

#include <stdint.h>

// clang-format off
// ============================= Tasks ======================================
template<size_t stack_size_, UBaseType_t priority_>
struct TaskDescriptor
{
    static constexpr size_t stack_size{stack_size_};
    static constexpr UBaseType_t priority{priority_};
    StackType_t stack[stack_size] {0UL};
    StaticTask_t task;
    TaskHandle_t handle{nullptr};

    result::Result init(TaskFunction_t function, const char * task_name, void * task_parameters)
    {
        handle = xTaskCreateStatic(
            function,
            task_name,
            stack_size,
            task_parameters,
            priority,
            stack,
            &task);
        if (nullptr == handle)
        {
            return result::Result::ERROR_GENERAL;
        }
 
        return result::Result::OK;
    }
};

TaskDescriptor<256, 1> audio_task;
TaskDescriptor<256, 1> log_task;
TaskDescriptor<256, 2> systemstate_task;

// ============================= Queues =====================================
template<typename T, size_t N>
struct QueueDescriptor
{
    using type = T;
    static constexpr size_t element_size{sizeof(T)};
    static constexpr size_t depth{N};

    StaticQueue_t queue;
    uint8_t buffer[N * sizeof(T)];
    QueueHandle_t handle{nullptr};

    result::Result init()
    {
        handle = xQueueCreateStatic( depth, element_size, buffer, &queue);
        if (nullptr == handle)
        {
            return result::Result::ERROR_GENERAL;
        }
        return result::Result::OK;
    }
};

QueueDescriptor<logger::CliCommandQueueElement, 1> cli_commands_queue;
QueueDescriptor<logger::CliStatusQueueElement, 1> cli_status_queue;

// ============================ Contexts ====================================
logger::CliContext      cli_context;
systemstate::Context    systemstate_context;

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

    cli_context.cli_commands_handle = cli_commands_queue.handle;
    cli_context.cli_status_handle = cli_status_queue.handle;

    // Tasks' initialization
    const auto audio_task_init_result = audio_task.init(audio::task_audio, "AUDIO", nullptr);
    if (result::Result::OK != audio_task_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    const auto log_task_init_result = log_task.init(logger::task_cli_logger, "CLI", &cli_context);
    if (result::Result::OK != log_task_init_result)
    {
        APP_ERROR_HANDLER(NRF_ERROR_NO_MEM);
    }

    systemstate_context.cli_commands_handle = cli_commands_queue.handle;
    systemstate_context.cli_status_handle = cli_status_queue.handle;

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
