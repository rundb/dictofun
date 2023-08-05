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
extern ble::fts::FileSystemInterface dictofun_test_fs_if;
}

namespace target
{

extern ble::fts::FileSystemInterface dictofun_fs_if;
void register_filesystem_queues(QueueHandle_t command_queue, QueueHandle_t status_queue, QueueHandle_t data_queue);
void register_keepalive_queue(QueueHandle_t keepalive_queue);

}
}
