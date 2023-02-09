// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#pragma once

#include "ble_fts.h"
#include "FreeRTOS.h"
#include "queue.h"

namespace integration
{
namespace test
{
result::Result dictofun_test_get_file_list(uint32_t& files_count, ble::fts::file_id_type * files_list_ptr);
result::Result dictofun_test_get_file_data(ble::fts::file_id_type file_id, uint8_t * file_data, uint32_t& file_data_size, uint32_t max_data_size);

extern ble::fts::FileSystemInterface dictofun_test_fs_if;
}

namespace target
{

extern ble::fts::FileSystemInterface dictofun_fs_if;
void register_filesystem_queues(QueueHandle_t command_queue, QueueHandle_t status_queue, QueueHandle_t data_queue);

}
}
