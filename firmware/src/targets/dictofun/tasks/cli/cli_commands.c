// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#include "cli_commands.h"

#include "nrf_cli.h"
#include "nrf_cli_uart.h"
#include "boards.h"

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