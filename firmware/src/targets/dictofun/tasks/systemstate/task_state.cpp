// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#include "task_state.h"
#include "result.h"

#include <nrf_gpio.h>
#include <nrf_log.h>
#include "nrf_log_ctrl.h"

#include "task_cli_logger.h"
#include "task_audio.h"
#include "task_audio_tester.h"
#include "task_memory.h"
#include "task_ble.h"
#include "task_led.h"
#include "task_button.h"

#include "nvconfig.h"

#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>
#include <queue.h>

namespace systemstate
{

logger::CliCommandQueueElement cli_command_buffer;
ble::RequestQueueElement ble_requests_buffer;
button::EventQueueElement button_event_buffer;

application::NvConfig _nvconfig{vTaskDelay};
application::NvConfig::Configuration _configuration;

constexpr TickType_t cli_command_wait_ticks_type{10};
constexpr TickType_t ble_request_wait_ticks_type{1};
constexpr TickType_t button_event_wait_ticks_type{1};

Context * context{nullptr};
static constexpr uint8_t power_flipflop_clk{26};
static constexpr uint8_t power_flipflop_d{27};

application::Mode get_operation_mode()
{
    return _configuration.mode;
}
    
bool is_record_start_by_cli_allowed(Context& context)
{
    if (context.is_record_active)
    {
        return false;
    }
    // TODO: add another check to see if CLI commands are overall allowed
    return true;
}

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

result::Result enable_ble_subsystem(Context& context)
{
    if (!context._is_ble_system_active)
    {
        ble::CommandQueueElement cmd{ble::Command::START};
        const auto ble_start_status = xQueueSend(
            context.ble_commands_handle,
            reinterpret_cast<void *>(&cmd), 
            0);
        if (ble_start_status != pdPASS)
        {
            NRF_LOG_ERROR("failed to queue BLE start operation");
            return result::Result::ERROR_GENERAL;
        }
        context._is_ble_system_active = true;
    }
    return result::Result::OK;
}

void record_end_callback(TimerHandle_t timer)
{
    context->is_record_active = false;
    audio::CommandQueueElement cmd{audio::Command::RECORD_STOP};
    xQueueSend(context->audio_commands_handle, reinterpret_cast<void *>(&cmd), 0);
    audio::StatusQueueElement response;
    static constexpr uint32_t file_closure_max_wait_time{500};
    const auto record_stop_status_result = xQueueReceive(context->audio_status_handle, reinterpret_cast<void *>(&response), file_closure_max_wait_time);
    if (pdTRUE != record_stop_status_result)
    {
        NRF_LOG_ERROR("state: timed out to wait record close");
    }

    if (context->_should_record_be_stored)
    {
        memory::CommandQueueElement cmd{memory::Command::CLOSE_WRITTEN_FILE, {0,0}};
        const auto create_file_res = xQueueSend(context->memory_commands_handle, reinterpret_cast<void *>(&cmd), 0);
        if (create_file_res != pdPASS)
        {
            NRF_LOG_ERROR("task state: failed to request file closure");
            return;
        }
    }
    else
    {
        audio::tester::ControlQueueElement tester_cmd;
        tester_cmd.should_enable_tester = false;
        [[maybe_unused]] const auto tester_start_status = xQueueSend(
            context->audio_tester_commands_handle,
            reinterpret_cast<void *>(&tester_cmd), 
            0);
    }
}

result::Result launch_record_timer(const TickType_t record_duration, Context& context)
{
    const auto period_change_result = xTimerChangePeriod(
        context.record_timer_handle,
        record_duration,
        0);

    if (pdPASS != period_change_result)
    {
        return result::Result::ERROR_GENERAL;
    }

    const auto timer_start_result = xTimerStart(context.record_timer_handle, 0);
    if (timer_start_result != pdPASS)
    {
        return result::Result::ERROR_GENERAL;
    }

    context.is_record_active = true;   
    return result::Result::OK;
}

void process_button_event(button::Event event);

void load_nvconfig(Context& context);

void task_system_state(void * context_ptr)
{
    configure_power_latch();
    NRF_LOG_INFO("task state: initialized");
    context = reinterpret_cast<Context *>(context_ptr);
    // Process NV configuration. If it doesn't exist - memory operation should be scheduled.
    const auto nvconfig_load_result = _nvconfig.load_early(_configuration);
    bool is_operation_mode_defined{true};
    static constexpr uint32_t nvconfig_definition_timestamp{2000};
    if (result::Result::OK != nvconfig_load_result)
    {
        NRF_LOG_WARNING("state: failed to early load application config");
        is_operation_mode_defined = false;
    }
    else
    {
        NRF_LOG_INFO("state: operation mode %s", _configuration.mode == application::DEVELOPMENT ? "DEV" : _configuration.mode == application::ENGINEERING ? "ENG" : "FIELD");
    }

    while(1)
    {
        const auto button_event_receive_status = xQueueReceive(
            context->button_events_handle,
            &button_event_buffer,
            button_event_wait_ticks_type
        );
        if (pdPASS == button_event_receive_status)
        {
            process_button_event(button_event_buffer.event);
        }

        const auto cli_queue_receive_status = xQueueReceive(
            context->cli_commands_handle,
            reinterpret_cast<void *>(&cli_command_buffer),
            cli_command_wait_ticks_type
        );
        if (pdPASS == cli_queue_receive_status)
        {
            switch (cli_command_buffer.command_id) 
            {
                case logger::CliCommand::RECORD:
                {
                    launch_cli_command_record(*context, cli_command_buffer.args[0], cli_command_buffer.args[1] > 0);
                    break;
                }
                case logger::CliCommand::MEMORY_TEST:
                {
                    launch_cli_command_memory_test(*context, cli_command_buffer.args[0]);
                    break;
                }
                case logger::CliCommand::BLE_COMMAND:
                {
                    launch_cli_command_ble_operation(*context, cli_command_buffer.args[0]);
                    break;
                }
                case logger::CliCommand::SYSTEM:
                {
                    launch_cli_command_system(*context, cli_command_buffer.args[0]);
                    break;
                }
                case logger::CliCommand::OPMODE:
                {
                    launch_cli_command_opmode(*context, cli_command_buffer.args[0], _nvconfig);
                    break;
                }
            }
        }
        const auto ble_request_receive_status = xQueueReceive(
            context->ble_requests_handle,
            reinterpret_cast<void *>(&ble_requests_buffer),
            ble_request_wait_ticks_type
        );
        if (pdPASS == ble_request_receive_status)
        {
            if (ble_requests_buffer.request == ble::Request::LED)
            {
                led::CommandQueueElement cmd{
                    led::user_color, 
                    (ble_requests_buffer.args[0] == 0) ? led::State::OFF : led::State::ON
                };
                const auto led_command_send_status = xQueueSend(
                    context->led_commands_handle,
                    reinterpret_cast<void *>(&cmd),
                    0
                );
                if (pdPASS != led_command_send_status)
                {
                    NRF_LOG_ERROR("state: failed to send led enable request");
                }
            }
        }
        if (!is_operation_mode_defined && xTaskGetTickCount() > nvconfig_definition_timestamp)
        {
            is_operation_mode_defined = true;
            load_nvconfig(*context);
        }
    }
}   

void process_button_event(button::Event event)
{
    // TODO: implement
}

/// @brief Unfortunately, nvconfig can't contain all dependencies within the module.
/// This happens because fstorage has to use SD-aware implementation, and in FreeRTOS-based system 
/// it works only if SD task is active. So in order to write-access fstorage we have to enable BLE
/// subsystem, if it's not enabled yet, and disable afterwards, if needed.
void load_nvconfig(Context& context)
{
    bool need_to_disable_ble_subsystem{false};
    if (!context._is_ble_system_active)
    {
        if (result::Result::OK != enable_ble_subsystem(context))
        {
            return;
        }
        need_to_disable_ble_subsystem = true;
        context._is_ble_system_active = true;
        // Wait to make sure BLE task has been started
        static constexpr uint32_t ble_start_wait_ticks{10};
        vTaskDelay(ble_start_wait_ticks);
    }

    const auto nvconfig_load_result = _nvconfig.load(_configuration);
    if (result::Result::OK != nvconfig_load_result)
    {
        NRF_LOG_ERROR("deferred load nvconfig has failed.");
    }

    if (need_to_disable_ble_subsystem)
    {
        ble::CommandQueueElement cmd{ble::Command::STOP};
        const auto ble_comm_status = xQueueSend(
            context.ble_commands_handle,
            reinterpret_cast<void *>(&cmd), 
            0);
        if (ble_comm_status != pdPASS)
        {
            NRF_LOG_ERROR("nvconfig: failed to queue BLE stop operation");
            return;
        }
        // FIXME: so far BLE subsystem can't be stopped/suspended due to how freertos SDH task API is implemented.
        //context._is_ble_system_active = false;
    }
}

}
