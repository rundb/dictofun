// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#pragma once

#include "FreeRTOS.h"
#include "queue.h"

#include "spi_flash.h"
#include "myfs.h"

namespace memory
{

// TODO: get rid of this `using` at the moment of merge request
using namespace filesystem;

void task_memory(void* context_ptr);
void launch_test_1(flash::SpiFlash& flash);
void launch_test_2(flash::SpiFlash& flash);

void launch_test_3(myfs_t& myfs, const myfs_config& myfs_configuration);
void launch_test_5(flash::SpiFlash& flash, const uint32_t range_start, const uint32_t range_end);

struct Context;
void generate_next_file_name(char* name, const Context& context);

enum class Command
{
    LAUNCH_TEST_1,
    LAUNCH_TEST_2,
    LAUNCH_TEST_3,
    LAUNCH_TEST_4,
    MOUNT_FS,
    UNMOUNT_FS,
    SELECT_OWNER_BLE,
    SELECT_OWNER_AUDIO,
    CREATE_RECORD,
    CLOSE_WRITTEN_FILE,
    PERFORM_MEMORY_CHECK,
    FORMAT_FS,

    LAUNCH_TEST_5, // memory range print
};

enum class Status
{
    OK,
    ERROR_BUSY,
    ERROR_GENERAL,
    FORMAT_REQUIRED,
};

struct CommandQueueElement
{
    Command command_id;
    static constexpr size_t max_arguments{2};
    uint32_t args[max_arguments]{0};
};

struct StatusQueueElement
{
    Command command_id;
    Status status;
};

struct Context
{
    QueueHandle_t audio_data_queue{nullptr};
    QueueHandle_t command_queue{nullptr};
    QueueHandle_t status_queue{nullptr};
    QueueHandle_t command_from_ble_queue{nullptr};
    QueueHandle_t status_to_ble_queue{nullptr};
    QueueHandle_t data_to_ble_queue{nullptr};
    QueueHandle_t commands_to_rtc_queue{nullptr};
    QueueHandle_t response_from_rtc_queue{nullptr};
};

} // namespace memory
