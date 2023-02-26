// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#include "task_cli_logger.h"

#include "FreeRTOS.h"
#include "task.h"
#include "boards.h"

#include <nrf_log.h>
#include <nrf_log_ctrl.h>
#include <nrf_log_default_backends.h>

#include "cli_commands.h"

namespace logger
{

static RecordLaunchCommand _record_launch_command;
void record_launch_callback(int duration, bool should_record_be_stored)
{
    if (_record_launch_command.is_active) return;
    _record_launch_command.duration = duration;
    _record_launch_command.should_be_stored = should_record_be_stored;
    _record_launch_command.is_active = true;
}

static MemoryTestCommand _memory_test_command;
void memory_test_callback(uint32_t test_id)
{
    if (_memory_test_command.is_active) return;
    _memory_test_command.test_id = test_id;
    _memory_test_command.is_active = true;
}

static BleOperationCommand _ble_operation_command;
void ble_operation_callback(uint32_t command_id)
{
    if (_ble_operation_command.is_active) return;
    _ble_operation_command.command_id = command_id;
    _ble_operation_command.is_active = true;
}

static SystemCommand _system_command;
void system_operation_callback(uint32_t command_id)
{
    if (_system_command.is_active) return;
    _system_command.command_id = command_id;
    _system_command.is_active = true;
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

void task_cli_logger(void * cli_context)
{
    NRF_LOG_INFO("task logger: initialized");
    CliContext& context = *(reinterpret_cast<CliContext *>(cli_context));
    register_record_launch_callback(record_launch_callback);
    register_memory_test_callback(memory_test_callback);
    register_ble_control_callback(ble_operation_callback);
    register_system_control_callback(system_operation_callback);
    while (1)
    {
        vTaskDelay(5);
        NRF_LOG_PROCESS();
        cli_process();
        if (_record_launch_command.is_active)
        {
            _record_launch_command.is_active = false;

            CliCommandQueueElement cmd{
                CliCommand::RECORD, 
                {
                    static_cast<uint32_t>(_record_launch_command.duration), 
                    static_cast<uint32_t>(_record_launch_command.should_be_stored)
                }
            };
            const auto send_result = xQueueSend(context.cli_commands_handle, &cmd, 0U);
            if (pdPASS != send_result)
            {
                NRF_LOG_WARNING("cli: failed to queue record operation");
            }
            else
            {
                NRF_LOG_INFO("cli: record launch operation has been queued");
            }
        }

        if (_memory_test_command.is_active)
        {
            _memory_test_command.is_active = false;
            CliCommandQueueElement cmd{
                CliCommand::MEMORY_TEST, 
                { static_cast<uint32_t>(_memory_test_command.test_id), 0 }
            };
            const auto send_result = xQueueSend(context.cli_commands_handle, &cmd, 0U);
            if (pdPASS != send_result)
            {
                NRF_LOG_WARNING("cli: failed to launch memory test");
            }
            else
            {
                NRF_LOG_INFO("cli: memory test operation has been queued");
            }
        }

        if (_ble_operation_command.is_active)
        {
            _ble_operation_command.is_active = false;
            CliCommandQueueElement cmd{
                CliCommand::BLE_COMMAND, 
                { static_cast<uint32_t>(_ble_operation_command.command_id), 0 }
            };
            const auto send_result = xQueueSend(context.cli_commands_handle, &cmd, 0U);
            if (pdPASS != send_result)
            {
                NRF_LOG_WARNING("cli: failed to launch BLE command");
            }
            else
            {
                NRF_LOG_INFO("cli: BLE command has been queued");
            }
        }
        if (_system_command.is_active)
        {
            _system_command.is_active = false;
            CliCommandQueueElement cmd{
                CliCommand::SYSTEM, 
                { static_cast<uint32_t>(_system_command.command_id), 0 }
            };
            const auto send_result = xQueueSend(context.cli_commands_handle, &cmd, 0U);
            if (pdPASS != send_result)
            {
                NRF_LOG_WARNING("cli: failed to launch system command");
            }
            else
            {
                NRF_LOG_INFO("cli: system command has been queued");
            }
        }
    }
}

}
