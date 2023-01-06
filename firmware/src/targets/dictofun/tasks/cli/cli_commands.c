// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

// Note: unfortunately, this functionality has to be compiled with a C compiler due to a bug in Nordic SDK. 

#include "cli_commands.h"

#include "nrf_cli.h"
#include "nrf_cli_uart.h"
#include "boards.h"

#include <string.h>

NRF_CLI_UART_DEF(m_cli_uart_transport, 0, 64, 16);
NRF_CLI_DEF(m_cli_uart, "> ", &m_cli_uart_transport.transport, '\r', CLI_LOG_QUEUE_SIZE);

void cli_uart_start()
{
    ret_code_t ret;

    ret = nrf_cli_start(&m_cli_uart);
    APP_ERROR_CHECK(ret);
}

void cli_uart_cfg()
{
    ret_code_t            err_code;
    nrf_drv_uart_config_t uart_config = NRF_DRV_UART_DEFAULT_CONFIG;

    uart_config.pseltxd = UART_TX_PIN_NUMBER;
    uart_config.pselrxd = UART_RX_PIN_NUMBER;
    uart_config.hwfc    = NRF_UART_HWFC_DISABLED;
    err_code            = nrf_cli_init(&m_cli_uart, &uart_config, true, true, NRF_LOG_SEVERITY_INFO);
    APP_ERROR_CHECK(err_code);
}


void cli_init()
{
    cli_uart_cfg();
    cli_uart_start();
}

void cli_process()
{
    nrf_cli_process(&m_cli_uart);
}

static void cmd_version(nrf_cli_t const * p_cli, size_t argc, char ** argv)
{
    if (nrf_cli_help_requested(p_cli))
    {
        nrf_cli_help_print(p_cli, NULL, 0);
        return;
    }

    nrf_cli_fprintf(p_cli,
                NRF_CLI_NORMAL,
                "placeholder for version printout\n");

}

NRF_CLI_CMD_REGISTER(version,
                     NULL,
                     "Display software version",
                     cmd_version);


static record_launch_callback _record_launch_callback = NULL;

void register_record_launch_callback(record_launch_callback callback)
{
    _record_launch_callback = callback;
}


/// @brief Launch a record of fixed length. Command accepts 2 arguments: duration in seconds and 
///        1 if record should be saved to storage, 0 if not (by default it's not saved)
static void cmd_record(nrf_cli_t const * p_cli, const size_t argc, char ** argv)
{
    if ((1 == argc) || (argc > 3))
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Wrong command syntax\n");
        return;
    }

    const int duration = atoi(argv[1]);
    if (duration <= 0)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Wrong record duration\n", argc);
        return;
    }

    bool should_record_be_stored = false;
    if (3 == argc)
    {
        if ((argv[2][0] == '1') && (atoi(argv[2]) == 1))
        {
            should_record_be_stored = true;
        }
        else if ((argv[2][0] == '0') && (atoi(argv[2]) == 0))
        {
            // do nothing here
        }
        else
        {
            nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Wrong storage flag (must be 0 or 1)\n", argc);
            return;
        }
    }

    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Record launch. Args: %d, %d\n", duration, (int)should_record_be_stored);
    if (_record_launch_callback != NULL)
    {
        _record_launch_callback(duration, should_record_be_stored);
        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Record launched for %d seconds and will %sbe stored\n", 
            duration, (should_record_be_stored ? "" : "not "));
    }
    else
    {
        nrf_cli_fprintf(p_cli,
            NRF_CLI_WARNING,
            "No record launch function registered. Command shall not be launched\n");
    }
}

NRF_CLI_CMD_REGISTER(record,
                     NULL,
                     "Launch record",
                     cmd_record);
