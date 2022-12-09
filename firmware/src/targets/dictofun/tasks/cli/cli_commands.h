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

#ifdef __cplusplus
}
#endif

