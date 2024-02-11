// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "task_state.h"

#include "task_audio.h"
#include "task_audio_tester.h"
#include "task_ble.h"
#include "task_led.h"
#include "task_memory.h"

#include "nrf_log.h"

namespace systemstate
{
void launch_cli_command_record(Context& context,
                               const uint32_t duration,
                               bool should_store_the_record)
{
    NRF_LOG_INFO("task: received command from CLI");
    if(is_record_start_by_cli_allowed(context))
    {
        context.should_record_be_stored = should_store_the_record;
        if(should_store_the_record)
        {
            const auto creation_result = request_record_creation(context);
            if(decltype(creation_result)::OK != creation_result)
            {
                NRF_LOG_ERROR("state: failed to create record per CLI request");
                return;
            }
        }

        const auto record_start_status = request_record_start(context);
        if(decltype(record_start_status)::OK != record_start_status)
        {
            NRF_LOG_ERROR("task state: failed to queue start_record command (launched by cli)");
            return;
        }

        // record in mode without storage, so audio tester should be enabled
        if(!should_store_the_record)
        {
            NRF_LOG_INFO("task state: enabling audio tester")
            audio::tester::ControlQueueElement cmd;
            cmd.should_enable_tester = true;
            [[maybe_unused]] const auto tester_start_status =
                xQueueSend(context.audio_tester_commands_handle, reinterpret_cast<void*>(&cmd), 0);
        }

        constexpr TickType_t ticks_per_second{1000};
        const TickType_t duration_ticks{duration * ticks_per_second};
        const auto timer_launch_result = launch_record_timer(duration_ticks, context);
        if(result::Result::OK != timer_launch_result)
        {
            NRF_LOG_ERROR("task state: failed to launch record stop timer");
        }
    }
    else
    {
        NRF_LOG_ERROR("task_state: record start is not allowed. Aborting");
    }
}

void launch_cli_command_memory_test(Context& context, const uint32_t test_id, const uint32_t range_start, const uint32_t range_end)
{
    NRF_LOG_INFO("task state: launching memory test %d", test_id);
    const memory::Command command_id = (test_id == 5)   ? memory::Command::LAUNCH_TEST_5
                                       : (test_id == 4)   ? memory::Command::LAUNCH_TEST_4
                                       : (test_id == 3) ? memory::Command::LAUNCH_TEST_3
                                       : (test_id == 2) ? memory::Command::LAUNCH_TEST_2
                                                        : memory::Command::LAUNCH_TEST_1;
    
    const uint32_t arg0 = (test_id == 5) ? range_start : 0;
    const uint32_t arg1 = (test_id == 5) ? range_end : 0;
    memory::CommandQueueElement cmd{command_id, {arg0, arg1}};
    const auto memtest_status =
        xQueueSend(context.memory_commands_handle, reinterpret_cast<void*>(&cmd), 0);
    if(memtest_status != pdPASS)
    {
        NRF_LOG_ERROR("task state: failed to queue memtest command");
        return;
    }
}

void launch_cli_command_ble_operation(Context& context, const uint32_t command_id)
{
    NRF_LOG_INFO("task state: launching BLE command %d", command_id);
    const ble::Command command = (command_id == 4)   ? ble::Command::CONNECT_FS
                                 : (command_id == 3) ? ble::Command::RESET_PAIRING
                                 : (command_id == 2) ? ble::Command::STOP
                                                     : ble::Command::START;
    if(command == ble::Command::CONNECT_FS)
    {
        // When connection to filesystem is established, ownership of memory should be changed
        memory::CommandQueueElement command_to_memory{memory::Command::SELECT_OWNER_BLE, {0, 0}};
        xQueueSend(context.memory_commands_handle, reinterpret_cast<void*>(&command_to_memory), 0);
    }

    ble::CommandQueueElement cmd{command};
    const auto ble_comm_status =
        xQueueSend(context.ble_commands_handle, reinterpret_cast<void*>(&cmd), 0);
    if(ble_comm_status != pdPASS)
    {
        NRF_LOG_ERROR("task state: failed to queue BLE operation");
        return;
    }
    if(command == ble::Command::START)
    {
        context.is_ble_system_active = true;
    }
    else if(command == ble::Command::STOP)
    {
        // FIXME: so far BLE subsystem can't be stopped/suspended due to how freertos SDH task API is implemented.
        // context._is_ble_system_active = false;
    }
}

void launch_cli_command_system(Context& context, const uint32_t command_id)
{
    NRF_LOG_INFO("task state: launching system command %d", command_id);
    shutdown_ldo();
}

void launch_cli_command_opmode(Context& context, const uint32_t mode_id, application::NvConfig& nvc)
{
    NRF_LOG_INFO("setting opmode to %s", mode_id == 1 ? "DEV" : (mode_id == 2) ? "ENG" : "FIELD");
    application::NvConfig::Configuration config;
    config.mode = (mode_id == 1)   ? application::Mode::DEVELOPMENT
                  : (mode_id == 2) ? application::Mode::ENGINEERING
                                   : application::Mode::FIELD;
    // Awkward dependency: BLE subsystem has to be started in order to launch this operation
    if(result::Result::OK != enable_ble_subsystem(context))
    {
        NRF_LOG_ERROR("opmode: failed to enable BLE as a pre-condition to modify nvconfig")
    }
    const auto config_change_result = nvc.store(config);
    if(result::Result::OK != config_change_result)
    {
        NRF_LOG_ERROR("state: failed to change opmode");
    }
    else
    {
        NRF_LOG_DEBUG("state: successfully updated opmode");
    }
}

void launch_cli_command_led(Context& context, const uint32_t color_id, const uint32_t mode_id)
{
    NRF_LOG_INFO("setting led color %d to %d", color_id, mode_id);
    if(color_id >= static_cast<uint32_t>(led::Color::COUNT))
    {
        NRF_LOG_ERROR("state: wrong color id (%d >= %d)", color_id, led::Color::COUNT);
        return;
    }
    if(mode_id >= static_cast<uint32_t>(led::State::COUNT))
    {
        NRF_LOG_ERROR("state: wrong LED mode id (%d >= %d)", mode_id, led::State::COUNT);
        return;
    }
    const auto color = static_cast<led::Color>(color_id);
    const auto state = static_cast<led::State>(mode_id);
    led::CommandQueueElement cmd{color, state};
    const auto led_command_send_status =
        xQueueSend(context.led_commands_handle, reinterpret_cast<void*>(&cmd), 0);
    if(pdTRUE != led_command_send_status)
    {
        NRF_LOG_ERROR("state: failed to send message to LED");
    }
}

} // namespace systemstate
