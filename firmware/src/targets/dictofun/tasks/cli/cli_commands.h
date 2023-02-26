// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#pragma once

#include "nrf_cli.h"
#include "nrf_cli_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

void cli_init();
void cli_process();

typedef void (*record_launch_callback)(int, bool) ;
void register_record_launch_callback(record_launch_callback callback);

typedef void (*memory_test_callback)(uint32_t) ;
void register_memory_test_callback(memory_test_callback callback);

typedef void (*ble_control_callback)(uint32_t) ;
void register_ble_control_callback(ble_control_callback callback);

typedef void (*system_control_callback)(uint32_t) ;
void register_system_control_callback(system_control_callback callback);

#ifdef __cplusplus
}
#endif

