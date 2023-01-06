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

#ifdef __cplusplus
}
#endif

