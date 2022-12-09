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
    while (1)
    {
        vTaskDelay(10);
        NRF_LOG_PROCESS();
        cli_process();
    }
}

}
