// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "ble_fts_glue.h"
#include <cstdio>
#include "ble_fts.h"
#include "FreeRTOS.h"
#include "queue.h"

using namespace ble::fts;

namespace integration
{

namespace target
{

static QueueHandle_t * _command_to_fs_queue{nullptr};
static QueueHandle_t * _status_from_fs_queue{nullptr};
QueueHandle_t * _data_from_fs_queue{nullptr};

inline bool is_fs_communication_valid()
{
    return _command_to_fs_queue != nullptr && _status_from_fs_queue != nullptr && _data_from_fs_queue != nullptr;
}

void register_filesystem_queues(QueueHandle_t * command_queue, QueueHandle_t * status_queue, QueueHandle_t * data_queue)
{
    _command_to_fs_queue = command_queue;
    _status_from_fs_queue = status_queue;
    _data_from_fs_queue = data_queue;
}

result::Result get_file_list(uint32_t& files_count, file_id_type * files_list_ptr)
{
    if (!is_fs_communication_valid())
    {
        return result::Result::ERROR_GENERAL;
    }
    return result::Result::ERROR_NOT_IMPLEMENTED;
}

result::Result get_file_info(file_id_type file_id, uint8_t * file_data, uint32_t& file_data_size, const uint32_t max_file_data_size)
{
    if (!is_fs_communication_valid())
    {
        return result::Result::ERROR_GENERAL;
    }
    return result::Result::ERROR_NOT_IMPLEMENTED;
}

result::Result open_file(file_id_type file_id, uint32_t& file_size)
{
    if (!is_fs_communication_valid())
    {
        return result::Result::ERROR_GENERAL;
    }
    return result::Result::ERROR_NOT_IMPLEMENTED;
}

result::Result close_file(file_id_type file_id)
{
    if (!is_fs_communication_valid())
    {
        return result::Result::ERROR_GENERAL;
    }
    return result::Result::ERROR_NOT_IMPLEMENTED;
}

result::Result get_data(file_id_type file_id, uint8_t * buffer, uint32_t& actual_size, uint32_t max_size)
{
    if (!is_fs_communication_valid())
    {
        return result::Result::ERROR_GENERAL;
    }
    return result::Result::ERROR_NOT_IMPLEMENTED;
}

result::Result fs_status(FileSystemInterface::FSStatus& status)
{
    if (!is_fs_communication_valid())
    {
        return result::Result::ERROR_GENERAL;
    }
    return result::Result::ERROR_NOT_IMPLEMENTED;
}

FileSystemInterface dictofun_fs_if
{
    get_file_list,
    get_file_info,
    open_file,
    close_file,
    get_data,
    fs_status
};

}
}
