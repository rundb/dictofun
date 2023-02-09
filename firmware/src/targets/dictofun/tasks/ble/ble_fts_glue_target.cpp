// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "ble_fts_glue.h"
#include <cstdio>
#include "ble_fts.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task_ble.h"
#include "nrf_log.h"

using namespace ble::fts;

namespace integration
{

namespace target
{

static QueueHandle_t _command_to_fs_queue{nullptr};
static QueueHandle_t _status_from_fs_queue{nullptr};
static QueueHandle_t _data_from_fs_queue{nullptr};

inline bool is_fs_communication_valid()
{
    return _command_to_fs_queue != nullptr && _status_from_fs_queue != nullptr && _data_from_fs_queue != nullptr;
}

void register_filesystem_queues(QueueHandle_t command_queue, QueueHandle_t status_queue, QueueHandle_t data_queue)
{
    _command_to_fs_queue = command_queue;
    _status_from_fs_queue = status_queue;
    _data_from_fs_queue = data_queue;
    NRF_LOG_DEBUG("ble fts: setting fs comm queue");
}

result::Result get_file_list(uint32_t& files_count, file_id_type * files_list_ptr)
{
    static constexpr uint32_t max_status_wait_time{100};
    static constexpr uint32_t max_data_wait_time{30};
    if (!is_fs_communication_valid())
    {
        return result::Result::ERROR_GENERAL;
    }
    ble::CommandToMemoryQueueElement cmd{ble::CommandToMemory::GET_FILES_LIST};
    ble::StatusFromMemoryQueueElement response;

    NRF_LOG_DEBUG("ble: sending command to flash memory");
    const auto cmd_result = xQueueSend(_command_to_fs_queue, &cmd, 0);
    if (pdTRUE != cmd_result)
    {
        NRF_LOG_ERROR("get file list: failed to send cmd to mem");
        return result::Result::ERROR_GENERAL;
    }
    const auto status_result = xQueueReceive(_status_from_fs_queue, &response, max_status_wait_time);
    if (pdTRUE != status_result)
    {
        NRF_LOG_ERROR("get file list: timed out recv status from mem");
        return result::Result::ERROR_GENERAL; 
    }
    if (response.status != ble::StatusFromMemory::OK)
    {
        NRF_LOG_ERROR("get file list: recv error status(%d)", static_cast<int>(response.status));
        return result::Result::ERROR_GENERAL;
    }
    else
    {
        NRF_LOG_DEBUG("ble::fts: received OK response from mem, waiting for the data");
    }

    // TODO: consider moving data element to a static memory, for performance reasons
    ble::FileDataFromMemoryQueueElement data;

    // TODO: make sure to test cases of both short lists (fitting to the data element) and longer lists
    const auto data_result = xQueueReceive(_data_from_fs_queue, &data, max_data_wait_time);
    if (pdTRUE != data_result)
    {
        NRF_LOG_ERROR("get file list: timed out recv data from mem");
        return result::Result::ERROR_GENERAL;
    }

    // TODO: at this point send it to the BLE
    NRF_LOG_DEBUG("glue: %d", data.size);
    
    files_count = data.size / sizeof(file_id_type);
    memcpy(files_list_ptr, data.data, data.size);
    return result::Result::OK;
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
