// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#include "task_cli_logger.h"

#include "FreeRTOS.h"
#include "boards.h"
#include "task.h"

#include <nrf_log.h>
#include <nrf_log_ctrl.h>
#include <nrf_log_default_backends.h>

#include "cli_commands.h"

#include "version.h"

namespace logger
{

static RecordLaunchCommand _record_launch_command;
void record_launch_callback(int duration, bool should_record_be_stored)
{
    if(_record_launch_command.is_active)
        return;
    _record_launch_command.duration = duration;
    _record_launch_command.should_be_stored = should_record_be_stored;
    _record_launch_command.is_active = true;
}

static MemoryTestCommand _memory_test_command;
void memory_test_callback(uint32_t test_id, uint32_t range_start, uint32_t range_end)
{
    if(_memory_test_command.is_active)
        return;
    _memory_test_command.test_id = test_id;
    _memory_test_command.range_start = range_start;
    _memory_test_command.range_end = range_end;
    _memory_test_command.is_active = true;
}

static BleOperationCommand _ble_operation_command;
void ble_operation_callback(uint32_t command_id)
{
    if(_ble_operation_command.is_active)
        return;
    _ble_operation_command.command_id = command_id;
    _ble_operation_command.is_active = true;
}

static SystemCommand _system_command;
void system_operation_callback(uint32_t command_id)
{
    if(_system_command.is_active)
        return;
    _system_command.command_id = command_id;
    _system_command.is_active = true;
}

static OpmodeConfigCommand _opmode_config_command;
void opmode_config_callback(uint32_t mode_id)
{
    if(_opmode_config_command.is_active)
        return;
    _opmode_config_command.is_active = true;
    _opmode_config_command.mode_id = mode_id;
}

static LedControlCommand _led_control_command;
void led_control_callback(uint32_t color_id, uint32_t mode_id)
{
    if(_led_control_command.is_active)
        return;
    _led_control_command.is_active = true;
    _led_control_command.color_id = color_id;
    _led_control_command.mode_id = mode_id;
}

uint32_t get_timestamp()
{
    return xTaskGetTickCount();
}

void log_init()
{
    ret_code_t err_code = NRF_LOG_INIT(get_timestamp);
    APP_ERROR_CHECK(err_code);
    cli_init();
}

void task_cli_logger(void* cli_context)
{
    NRF_LOG_INFO("%s", version::BUILD_SUMMARY_STRING);
    CliContext& context = *(reinterpret_cast<CliContext*>(cli_context));
    register_record_launch_callback(record_launch_callback);
    register_memory_test_callback(memory_test_callback);
    register_ble_control_callback(ble_operation_callback);
    register_system_control_callback(system_operation_callback);
    register_opmode_config_callback(opmode_config_callback);
    register_led_control_callback(led_control_callback);
    while(1)
    {
        NRF_LOG_PROCESS();
        vTaskDelay(5);
        cli_process();
        if(_record_launch_command.is_active)
        {
            _record_launch_command.is_active = false;

            CliCommandQueueElement cmd{
                CliCommand::RECORD,
                {static_cast<uint32_t>(_record_launch_command.duration),
                 static_cast<uint32_t>(_record_launch_command.should_be_stored)}};
            const auto send_result = xQueueSend(context.cli_commands_handle, &cmd, 0U);
            if(pdPASS != send_result)
            {
                NRF_LOG_WARNING("cli: failed to queue record operation");
            }
            else
            {
                NRF_LOG_INFO("cli: record launch operation has been queued");
            }
        }

        if(_memory_test_command.is_active)
        {
            _memory_test_command.is_active = false;
            CliCommandQueueElement cmd{CliCommand::MEMORY_TEST, 
                {
                    _memory_test_command.test_id, 
                    _memory_test_command.range_start,
                    _memory_test_command.range_end,
                }
            };
            const auto send_result = xQueueSend(context.cli_commands_handle, &cmd, 0U);
            if(pdPASS != send_result)
            {
                NRF_LOG_WARNING("cli: failed to launch memory test");
            }
            else
            {
                NRF_LOG_INFO("cli: memory test operation has been queued");
            }
        }

        if(_ble_operation_command.is_active)
        {
            _ble_operation_command.is_active = false;
            CliCommandQueueElement cmd{CliCommand::BLE_COMMAND,
                                       {_ble_operation_command.command_id, 0}};
            const auto send_result = xQueueSend(context.cli_commands_handle, &cmd, 0U);
            if(pdPASS != send_result)
            {
                NRF_LOG_WARNING("cli: failed to launch BLE command");
            }
            else
            {
                NRF_LOG_INFO("cli: BLE command has been queued");
            }
        }
        if(_system_command.is_active)
        {
            _system_command.is_active = false;
            CliCommandQueueElement cmd{CliCommand::SYSTEM, {_system_command.command_id, 0}};
            const auto send_result = xQueueSend(context.cli_commands_handle, &cmd, 0U);
            if(pdPASS != send_result)
            {
                NRF_LOG_WARNING("cli: failed to launch system command");
            }
            else
            {
                NRF_LOG_INFO("cli: system command has been queued");
            }
        }
        if(_opmode_config_command.is_active)
        {
            _opmode_config_command.is_active = false;
            CliCommandQueueElement cmd{CliCommand::OPMODE, {_opmode_config_command.mode_id, 0}};
            const auto send_result = xQueueSend(context.cli_commands_handle, &cmd, 0U);
            if(pdPASS != send_result)
            {
                NRF_LOG_WARNING("cli: failed to launch opmode command");
            }
            else
            {
                NRF_LOG_INFO("cli: opmode config command has been queued");
            }
        }
        if(_led_control_command.is_active)
        {
            _led_control_command.is_active = false;
            CliCommandQueueElement cmd{
                CliCommand::LED, {_led_control_command.color_id, _led_control_command.mode_id}};
            const auto send_result = xQueueSend(context.cli_commands_handle, &cmd, 0U);
            if(pdPASS != send_result)
            {
                NRF_LOG_WARNING("cli: failed to launch led command");
            }
            else
            {
                NRF_LOG_INFO("cli: led control command has been queued");
            }
        }
    }
}

} // namespace logger
