// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#include "task_state.h"
#include "BleSystem.h"
#include "spi_flash.h"
#include "result.h"

#include <app_timer.h>
#include <nrf_gpio.h>
#include <nrf_log.h>
#include "battery_measurement.h"

#include "task_cli_logger.h"
#include "task_audio.h"
#include "task_audio_tester.h"

#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>
#include <queue.h>

namespace systemstate
{

logger::CliCommandQueueElement cli_command_buffer;

constexpr TickType_t cli_command_wait_ticks_type{10};

Context * context{nullptr};
    
bool is_record_start_by_cli_allowed()
{
    if (context->is_record_active)
    {
        return false;
    }
    // TODO: add another check to see if CLI commands are overall allowed
    return true;
}

void record_end_callback(TimerHandle_t timer)
{
    context->is_record_active = false;
    audio::CommandQueueElement cmd{audio::Command::RECORD_STOP};
    const auto record_stop_status = xQueueSend(
        context->audio_commands_handle,
        reinterpret_cast<void *>(&cmd), 
        0);

    audio::tester::ControlQueueElement tester_cmd;
    tester_cmd.should_enable_tester = false;
    const auto tester_start_status = xQueueSend(
        context->audio_tester_commands_handle,
        reinterpret_cast<void *>(&tester_cmd), 
        0);
}

result::Result launch_record_timer(const TickType_t record_duration)
{
    const auto period_change_result = xTimerChangePeriod(
        context->record_timer_handle,
        record_duration,
        0);

    if (pdPASS != period_change_result)
    {
        return result::Result::ERROR_GENERAL;
    }

    const auto timer_start_result = xTimerStart(context->record_timer_handle, 0);
    if (timer_start_result != pdPASS)
    {
        return result::Result::ERROR_GENERAL;
    }

    context->is_record_active = true;   
    return result::Result::OK;
}

void task_system_state(void * context_ptr)
{
    NRF_LOG_INFO("task state: initialized");
    context = reinterpret_cast<Context *>(context_ptr);
    while(1)
    {
        const auto cli_queue_receive_status = xQueueReceive(
            context->cli_commands_handle,
            reinterpret_cast<void *>(&cli_command_buffer),
            cli_command_wait_ticks_type
        );
        if (pdPASS == cli_queue_receive_status)
        {
            NRF_LOG_INFO("task: received command from CLI");
            if (is_record_start_by_cli_allowed())
            {
                audio::CommandQueueElement cmd{audio::Command::RECORD_START};
                const auto record_start_status = xQueueSend(
                    context->audio_commands_handle,
                    reinterpret_cast<void *>(&cmd), 
                    0);
                if (record_start_status != pdPASS)
                {
                    NRF_LOG_ERROR("task state: failed to queue start_record command");
                    continue;
                }

                // record in mode without storage, so audio tester should be enabled
                if (cli_command_buffer.args[1] == 0)
                {
                    NRF_LOG_INFO("task state: enabling audio tester")
                    audio::tester::ControlQueueElement cmd;
                    cmd.should_enable_tester = true;
                    const auto tester_start_status = xQueueSend(
                        context->audio_tester_commands_handle,
                        reinterpret_cast<void *>(&cmd), 
                        0);
                }
                else
                {
                    // enable the storage module, if necessary
                }
                
                constexpr TickType_t ticks_per_second{1000};
                const TickType_t duration{cli_command_buffer.args[0] * ticks_per_second};
                const auto timer_launch_result = launch_record_timer(duration);
                if (result::Result::OK != timer_launch_result)
                {
                    NRF_LOG_ERROR("task state: failed to launch record stop timer");
                }
            }
            else
            {
                NRF_LOG_ERROR("task_state: record start is not allowed. Aborting");
            }
        }
    }
}   
}
