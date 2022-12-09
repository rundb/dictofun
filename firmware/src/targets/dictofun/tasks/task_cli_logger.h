// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#pragma once

namespace logger
{
/// Logger, CLI and required OS infrastructure

void log_init();

/// FreeRTOS task utilized for Logger and CLI
void task_cli_logger(void *);
}
