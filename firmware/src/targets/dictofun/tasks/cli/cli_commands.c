// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

// Note: unfortunately, this functionality has to be compiled with a C compiler due to a bug in Nordic SDK. 

#include "cli_commands.h"

#include "nrf_cli.h"
#include "nrf_cli_uart.h"
#include "boards.h"

#include "FreeRTOS.h"
#include "task.h"

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
                "dictofun application\n");

}
NRF_CLI_CMD_REGISTER(version, NULL, "Display software version", cmd_version);

static void cmd_reset(nrf_cli_t const * p_cli, size_t argc, char ** argv)
{
    if (nrf_cli_help_requested(p_cli))
    {
        nrf_cli_help_print(p_cli, NULL, 0);
        return;
    }

    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "resetting\n");
    vTaskDelay(100);
    NVIC_SystemReset();
}

NRF_CLI_CMD_REGISTER(reset, NULL, "Perform a software reset", cmd_reset);

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

NRF_CLI_CMD_REGISTER(record, NULL, "Launch record", cmd_record);

static memory_test_callback _memory_test_callback = NULL;
void register_memory_test_callback(memory_test_callback callback)
{
    _memory_test_callback = callback;
}

/// @brief Launch a set of tests that check operation of storage subsystem.
/// Syntax: memory N, where N is the test identifier
/// Test 1: erase the whole flash chip and print a message when erase is completed
/// Test 2: erase a sector in the end of flash memory (second from the end), write a page of data and read it back.
///         Print test status and duration afterwards
/// Test 3: test file system: create a file, write data into it, close it, open it for read back and validate the content. 
static void cmd_test_memory(nrf_cli_t const * p_cli, const size_t argc, char ** argv)
{
    if (2 != argc)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Wrong command syntax\n");
        return;
    }
    const int test_id = atoi(argv[1]);
    if (test_id < 1 || test_id > 4)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Wrong test ID\n", argc);
        return;
    }
    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Memory test %d launch\n", test_id);

    if (_memory_test_callback != NULL)
    {
        _memory_test_callback(test_id);
        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Memory test #%d is launched\n", test_id);
    }
    else
    {
        nrf_cli_fprintf(p_cli,
            NRF_CLI_WARNING,
            "No memory test launch function registered. Command shall not be launched\n");
    }
}

NRF_CLI_CMD_REGISTER(memtest, NULL, "Launch memory test", cmd_test_memory);

static ble_control_callback _ble_control_callback = NULL;
void register_ble_control_callback(ble_control_callback callback)
{
    _ble_control_callback = callback;
}

/// @brief Set of CLI commands for control over BLE subsystem operation
/// Following commands that are currently supported:
/// 1. enable BLE operation
/// 2. stop BLE operation
/// 3. reset pairing - clears pairing information (currently unsupported)
/// 4. connect real file system to the ble
static void cmd_ble_commands(nrf_cli_t const * p_cli, const size_t argc, char ** argv)
{
    if (argc < 2 || argc > 4)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Wrong command syntax\n");
        return;
    }
    // TODO: add support for the second argument, if needed
    const int command_id = atoi(argv[1]);
    if (command_id < 1 || command_id > 4)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Wrong command ID\n", argc);
        return;
    }
    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Launching BLE command\n");
    if (_ble_control_callback != NULL)
    {
        _ble_control_callback(command_id);
        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "BLE command #%d is launched\n", command_id);
    }
    else
    {
        nrf_cli_fprintf(p_cli,
            NRF_CLI_WARNING,
            "No BLE command launch function registered. Command shall not be launched\n");
    }
}

NRF_CLI_CMD_REGISTER(ble, NULL, "Launch BLE command", cmd_ble_commands);

static system_control_callback _system_control_callback = NULL;
void register_system_control_callback(system_control_callback callback)
{
    _system_control_callback = callback;
}

static void cmd_system_commands(nrf_cli_t const * p_cli, const size_t argc, char ** argv)
{
    if (argc != 2)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Wrong command syntax\n");
        return;
    }
    // TODO: add support for the second argument, if needed
    const int command_id = atoi(argv[1]);
    if (command_id != 1)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Wrong command ID\n", argc);
        return;
    }
    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Launching system command\n");
    if (_system_control_callback != NULL)
    {
        _system_control_callback(command_id);
        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "system command #%d is launched\n", command_id);
    }
    else
    {
        nrf_cli_fprintf(p_cli,
            NRF_CLI_WARNING,
            "No system command launch function registered. Command shall not be launched\n");
    }
}

NRF_CLI_CMD_REGISTER(system, NULL, "Launch system command", cmd_system_commands);

static opmode_config_callback _opmode_config_callback = NULL;
void register_opmode_config_callback(opmode_config_callback callback)
{
    _opmode_config_callback = callback;
}

static void cmd_opmode(nrf_cli_t const * p_cli, const size_t argc, char ** argv)
{
    if (argc != 2)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Wrong command syntax\n");
        return;
    }
    
    const int mode_id = atoi(argv[1]);
    if (mode_id < 1 || mode_id > 3)
    {
        nrf_cli_fprintf(p_cli, NRF_CLI_ERROR, "Wrong command ID\n", argc);
        return;
    }
    nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "Launching opmode change command\n");
    if (_opmode_config_callback != NULL)
    {
        _opmode_config_callback(mode_id);
        nrf_cli_fprintf(p_cli, NRF_CLI_NORMAL, "opmode #%d is launched\n", mode_id);
    }
    else
    {
        nrf_cli_fprintf(p_cli,
            NRF_CLI_WARNING,
            "No opmode command launch function registered. Command shall not be launched\n");
    }
}

NRF_CLI_CMD_REGISTER(opmode, NULL, "Change operation mode", cmd_opmode);
