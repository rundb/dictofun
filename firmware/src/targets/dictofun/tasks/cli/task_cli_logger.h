// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#pragma once

#include "FreeRTOS.h"
#include "queue.h"

namespace logger
{
/// Logger, CLI and required OS infrastructure

void log_init();

/// FreeRTOS task utilized for Logger and CLI
void task_cli_logger(void * cli_context);

struct CliContext
{
    QueueHandle_t cli_commands_handle{nullptr};
    QueueHandle_t cli_status_handle{nullptr};
};

struct RecordLaunchCommand
{
    bool is_active{false};
    int duration{0};
    bool should_be_stored{false};
};

struct MemoryTestCommand
{
    bool is_active{false};
    uint32_t test_id{0};
};

struct BleOperationCommand
{
    bool is_active{false};
    uint32_t command_id;
};

enum class CliCommand
{
    RECORD,
    MEMORY_TEST,
    BLE_COMMAND,
};

enum class CliStatus
{
    OK,
    ERROR,
};

struct CliCommandQueueElement
{
    static constexpr size_t max_arguments{2U};
    CliCommand command_id;
    uint32_t args[max_arguments];
};

struct CliStatusQueueElement
{
    CliCommand command_id;
    CliStatus status;
};

}
