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

#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>
#include <queue.h>

namespace systemstate
{

logger::CliCommandQueueElement cli_command_buffer;
ble::RequestQueueElement ble_requests_buffer;
bool _should_record_be_stored{false};

constexpr TickType_t cli_command_wait_ticks_type{10};
constexpr TickType_t ble_request_wait_ticks_type{1};

Context * context{nullptr};
static constexpr uint8_t power_flipflop_clk{26};
static constexpr uint8_t power_flipflop_d{27};
    
bool is_record_start_by_cli_allowed()
{
    if (context->is_record_active)
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

    if (_should_record_be_stored)
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
        const auto tester_start_status = xQueueSend(
            context->audio_tester_commands_handle,
            reinterpret_cast<void *>(&tester_cmd), 
            0);
    }
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

void launch_cli_command_record(const uint32_t duration, bool should_store_the_record);
void launch_cli_command_memory_test(const uint32_t test_id);
void launch_cli_command_ble_operation(const uint32_t command_id);
void launch_cli_command_system(const uint32_t command_id);

void task_system_state(void * context_ptr)
{
    configure_power_latch();
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
            switch (cli_command_buffer.command_id) 
            {
                case logger::CliCommand::RECORD:
                {
                    launch_cli_command_record(cli_command_buffer.args[0], cli_command_buffer.args[1] > 0);
                    break;
                }
                case logger::CliCommand::MEMORY_TEST:
                {
                    launch_cli_command_memory_test(cli_command_buffer.args[0]);
                    break;
                }
                case logger::CliCommand::BLE_COMMAND:
                {
                    launch_cli_command_ble_operation(cli_command_buffer.args[0]);
                    break;
                }
                case logger::CliCommand::SYSTEM:
                {
                    launch_cli_command_system(cli_command_buffer.args[0]);
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
    }
}   

void launch_cli_command_record(const uint32_t duration, bool should_store_the_record)
{
    NRF_LOG_INFO("task: received command from CLI");
    if (is_record_start_by_cli_allowed())
    {
        _should_record_be_stored = should_store_the_record;
        if (should_store_the_record)
        {
            memory::CommandQueueElement cmd{memory::Command::CREATE_RECORD, {0,0}};
            const auto create_file_res = xQueueSend(context->memory_commands_handle, reinterpret_cast<void *>(&cmd), 0);
            if (create_file_res != pdPASS)
            {
                NRF_LOG_ERROR("task state: failed to request file creation");
                return;
            }
            memory::StatusQueueElement response;
            static constexpr uint32_t file_creation_wait_time{500};
            const auto create_file_response_res = xQueueReceive(context->memory_status_handle, reinterpret_cast<void*>(&response), file_creation_wait_time);
            if (create_file_response_res != pdPASS)
            {
                NRF_LOG_ERROR("task state: response from mem timeout. Record won't be started");
                return;
            }
        }

        audio::CommandQueueElement cmd{audio::Command::RECORD_START};
        const auto record_start_status = xQueueSend(
            context->audio_commands_handle,
            reinterpret_cast<void *>(&cmd), 
            0);
        if (record_start_status != pdPASS)
        {
            NRF_LOG_ERROR("task state: failed to queue start_record command");
            return;
        }

        // record in mode without storage, so audio tester should be enabled
        if (!should_store_the_record)
        {
            NRF_LOG_INFO("task state: enabling audio tester")
            audio::tester::ControlQueueElement cmd;
            cmd.should_enable_tester = true;
            const auto tester_start_status = xQueueSend(
                context->audio_tester_commands_handle,
                reinterpret_cast<void *>(&cmd), 
                0);
        }
        
        constexpr TickType_t ticks_per_second{1000};
        const TickType_t duration_ticks{duration * ticks_per_second};
        const auto timer_launch_result = launch_record_timer(duration_ticks);
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

void launch_cli_command_memory_test(const uint32_t test_id)
{
    NRF_LOG_INFO("task state: launching memory test %d", test_id);
    const memory::Command command_id = 
        (test_id == 4) ? memory::Command::LAUNCH_TEST_4 :
        (test_id == 3) ? memory::Command::LAUNCH_TEST_3 :
        (test_id == 2) ? memory::Command::LAUNCH_TEST_2 :
        memory::Command::LAUNCH_TEST_1;
    memory::CommandQueueElement cmd{command_id, {0,0}};
    const auto memtest_status = xQueueSend(
        context->memory_commands_handle,
        reinterpret_cast<void *>(&cmd), 
        0);
    if (memtest_status != pdPASS)
    {
        NRF_LOG_ERROR("task state: failed to queue memtest command");
        return;
    }
}

void launch_cli_command_ble_operation(const uint32_t command_id)
{
    NRF_LOG_INFO("task state: launching BLE command %d", command_id);
    const ble::Command command = 
        (command_id == 4) ? ble::Command::CONNECT_FS :
        (command_id == 3) ? ble::Command::RESET_PAIRING :
        (command_id == 2) ? ble::Command::STOP :
        ble::Command::START;
    if (command == ble::Command::CONNECT_FS)
    {
        // When connection to filesystem is established, ownership of memory should be changed
        memory::CommandQueueElement command_to_memory{memory::Command::SELECT_OWNER_BLE, {0, 0}};
        xQueueSend(context->memory_commands_handle, reinterpret_cast<void *>(&command_to_memory), 0);
    }

    ble::CommandQueueElement cmd{command};
    const auto ble_comm_status = xQueueSend(
        context->ble_commands_handle,
        reinterpret_cast<void *>(&cmd), 
        0);
    if (ble_comm_status != pdPASS)
    {
        NRF_LOG_ERROR("task state: failed to queue BLE operation");
        return;
    }
}

void launch_cli_command_system(const uint32_t command_id)
{
    NRF_LOG_INFO("task state: launching system command %d", command_id);
    shutdown_ldo();
}

}

