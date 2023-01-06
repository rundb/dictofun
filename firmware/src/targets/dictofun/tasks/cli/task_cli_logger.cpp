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

void task_cli_logger(void *)
{
    NRF_LOG_INFO("task logger: initialized");
    register_record_launch_callback(record_launch_callback);
    while (1)
    {
        vTaskDelay(5);
        NRF_LOG_PROCESS();
        cli_process();
        if (_record_launch_command.is_active)
        {
            _record_launch_command.is_active = false;
            // TODO: using a queue, send this command to the task_state
            NRF_LOG_INFO("launching record");
        }
    }
}

}
