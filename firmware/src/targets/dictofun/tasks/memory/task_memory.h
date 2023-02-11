// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#pragma once

#include "FreeRTOS.h"
#include "queue.h"

#include "spi_flash.h"
#include "lfs.h"

namespace memory
{
bool isMemoryErased();


void task_memory(void * context_ptr);
void launch_test_1(flash::SpiFlash& flash);
void launch_test_2(flash::SpiFlash& flash);
void launch_test_3(lfs_t& lfs, const lfs_config& lfs_configuration);

enum class Command
{
    LAUNCH_TEST_1,
    LAUNCH_TEST_2,
    LAUNCH_TEST_3,
    FORMAT_CONFIG,
    OPEN_FILE_FOR_WRITE,
    CLOSE_FILE_FOR_WRITE,
    OPEN_FILE_FOR_READ,
    CLOSE_FILE_FOR_READ,
    INVALIDATE_FILE,
};

enum class Status
{
    OK,
    ERROR_BUSY,
    ERROR_GENERAL,
};

struct CommandQueueElement
{
    Command command_id;
    static constexpr size_t max_arguments{2};
    uint32_t args[max_arguments];
};

struct StatusQueueElement
{
    Command command_id;
    Status status;
};

struct Context
{
    QueueHandle_t audio_data_queue {nullptr};
    QueueHandle_t command_queue {nullptr};
    QueueHandle_t status_queue {nullptr};
    QueueHandle_t command_from_ble_queue {nullptr};
    QueueHandle_t status_to_ble_queue {nullptr};
    QueueHandle_t data_to_ble_queue {nullptr};

};

}
