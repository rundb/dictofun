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
#include "crc32.h"

using namespace ble::fts;

static constexpr uint32_t ROTU_max_idx{512};
extern uint8_t ROTU_data[ROTU_max_idx][2];
static uint32_t ROTU_idx{0};
static uint32_t ROTU_total_data{0};
uint32_t ROTU_total_data_start{0};
#define ROTU_PUSH_DATA(d) {if (ROTU_total_data > ROTU_total_data_start &&  ROTU_idx < ROTU_max_idx) {ROTU_data[ROTU_idx][0] = d; ++ROTU_idx;} }

namespace integration
{

namespace target
{

static QueueHandle_t _command_to_fs_queue{nullptr};
static QueueHandle_t _status_from_fs_queue{nullptr};
static QueueHandle_t _data_from_fs_queue{nullptr};

static constexpr uint32_t max_status_wait_time{400};
static constexpr uint32_t max_short_data_wait_time{100};
static constexpr uint32_t max_long_data_wait_time{400};

static uint32_t crc_value{0};
static bool is_first_frame_sent{false};

// this queue element is allocated statically, as it's rather huge (~260 bytes) and it's better to avoid putting it on stack
ble::FileDataFromMemoryQueueElement data_from_memory_queue_element;

inline bool is_fs_communication_valid()
{
    return _command_to_fs_queue != nullptr && _status_from_fs_queue != nullptr && _data_from_fs_queue != nullptr;
}

void register_filesystem_queues(QueueHandle_t command_queue, QueueHandle_t status_queue, QueueHandle_t data_queue)
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
    ble::CommandToMemoryQueueElement cmd{ble::CommandToMemory::GET_FILES_LIST};
    ble::StatusFromMemoryQueueElement response;

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
    }

    ble::FileDataFromMemoryQueueElement& data{data_from_memory_queue_element};

    // TODO: make sure to test cases of both short lists (fitting to the data element) and longer lists
    const auto data_result = xQueueReceive(_data_from_fs_queue, &data, max_short_data_wait_time);
    if (pdTRUE != data_result)
    {
        NRF_LOG_ERROR("get file list: timed out recv data from mem");
        return result::Result::ERROR_GENERAL;
    }
    
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
    if (file_data == nullptr || max_file_data_size == 0)
    {
        return result::Result::ERROR_INVALID_PARAMETER;
    }

    ble::CommandToMemoryQueueElement cmd{ble::CommandToMemory::GET_FILE_INFO, file_id};
    ble::StatusFromMemoryQueueElement response;

    const auto cmd_result = xQueueSend(_command_to_fs_queue, &cmd, 0);
    if (pdTRUE != cmd_result)
    {
        NRF_LOG_ERROR("get file info: failed to send cmd to mem");
        return result::Result::ERROR_GENERAL;
    }
    const auto status_result = xQueueReceive(_status_from_fs_queue, &response, max_status_wait_time);
    if (pdTRUE != status_result)
    {
        NRF_LOG_ERROR("get file info: timed out recv status from mem");
        return result::Result::ERROR_GENERAL; 
    }
    if (response.status != ble::StatusFromMemory::OK)
    {
        NRF_LOG_ERROR("get file info: recv error status(%d)", static_cast<int>(response.status));
        return result::Result::ERROR_GENERAL;
    }
    else
    {
    }
    ble::FileDataFromMemoryQueueElement& data{data_from_memory_queue_element};
    const auto data_result = xQueueReceive(_data_from_fs_queue, &data, max_short_data_wait_time);
    if (pdTRUE != data_result)
    {
        NRF_LOG_ERROR("get file info: timed out recv data from mem");
        return result::Result::ERROR_GENERAL;
    }
    
    file_data_size = data.size;
    memcpy(file_data, data.data, data.size);
    return result::Result::OK;
}

result::Result open_file(const file_id_type file_id, uint32_t& file_size)
{
    if (!is_fs_communication_valid())
    {
        return result::Result::ERROR_GENERAL;
    }

    ble::CommandToMemoryQueueElement cmd{ble::CommandToMemory::OPEN_FILE, file_id};
    ble::StatusFromMemoryQueueElement response;

    const auto cmd_result = xQueueSend(_command_to_fs_queue, &cmd, 0);
    if (pdTRUE != cmd_result)
    {
        NRF_LOG_ERROR("open file: failed to send cmd to mem");
        return result::Result::ERROR_GENERAL;
    }
    const auto status_result = xQueueReceive(_status_from_fs_queue, &response, max_status_wait_time);
    if (pdTRUE != status_result)
    {
        NRF_LOG_ERROR("open file: timed out recv status from mem");
        return result::Result::ERROR_GENERAL; 
    }
    if (response.status != ble::StatusFromMemory::OK)
    {
        NRF_LOG_ERROR("open file: recv error status(%d)", static_cast<int>(response.status));
        return result::Result::ERROR_GENERAL;
    }
    else
    {
    }
    NRF_LOG_DEBUG("opened file with size %d", response.data_size);
    file_size = response.data_size;
    is_first_frame_sent = false;
    return result::Result::OK;
}

result::Result close_file(const file_id_type file_id)
{
    if (!is_fs_communication_valid())
    {
        return result::Result::ERROR_GENERAL;
    }
    ble::CommandToMemoryQueueElement cmd{ble::CommandToMemory::CLOSE_FILE, file_id};
    ble::StatusFromMemoryQueueElement response;

    const auto cmd_result = xQueueSend(_command_to_fs_queue, &cmd, 0);
    if (pdTRUE != cmd_result)
    {
        NRF_LOG_ERROR("close file: failed to send cmd to mem");
        return result::Result::ERROR_GENERAL;
    }
    const auto status_result = xQueueReceive(_status_from_fs_queue, &response, max_status_wait_time);
    if (pdTRUE != status_result)
    {
        NRF_LOG_ERROR("close file: timed out recv status from mem");
        return result::Result::ERROR_GENERAL; 
    }
    if (response.status != ble::StatusFromMemory::OK)
    {
        NRF_LOG_ERROR("close file: recv error status(%d)", static_cast<int>(response.status));
        return result::Result::ERROR_GENERAL;
    }
    else
    {
    }
    NRF_LOG_DEBUG("closed file");
    NRF_LOG_INFO("glue: file crc: 0x%x, size %d", crc_value, ROTU_total_data);
    return result::Result::OK;
}

result::Result get_data(const file_id_type file_id, uint8_t * buffer, uint32_t& actual_size, const uint32_t max_size)
{
    if (!is_fs_communication_valid())
    {
        return result::Result::ERROR_GENERAL;
    }
    if (buffer == nullptr || max_size == 0)
    {
        return result::Result::ERROR_INVALID_PARAMETER;
    }

    xQueueReset(_data_from_fs_queue);

    ble::CommandToMemoryQueueElement cmd{ble::CommandToMemory::GET_FILE_DATA, file_id};
    ble::StatusFromMemoryQueueElement response;

    const auto cmd_result = xQueueSend(_command_to_fs_queue, &cmd, 0);
    if (pdTRUE != cmd_result)
    {
        NRF_LOG_ERROR("get file data: failed to send cmd to mem");
        return result::Result::ERROR_GENERAL;
    }
    const auto status_result = xQueueReceive(_status_from_fs_queue, &response, max_status_wait_time);
    if (pdTRUE != status_result)
    {
        NRF_LOG_ERROR("get file data: timed out recv status from mem");
        return result::Result::ERROR_GENERAL; 
    }
    if (response.status != ble::StatusFromMemory::OK)
    {
        NRF_LOG_ERROR("get file data: recv error status(%d)", static_cast<int>(response.status));
        return result::Result::ERROR_GENERAL;
    }
    else
    {
    }
    ble::FileDataFromMemoryQueueElement& data{data_from_memory_queue_element};
    
    data.size = 0;
    memset(&data, 0, sizeof(data));
    const auto data_result = xQueueReceive(_data_from_fs_queue, &data, max_long_data_wait_time);
    if (pdTRUE != data_result)
    {
        NRF_LOG_ERROR("get file data: timed out recv data from mem");
        return result::Result::ERROR_GENERAL;
    }
    
    actual_size = data.size;
    
    memcpy(buffer, data.data, data.size);

    crc_value = crc32_compute(data.data, data.size, is_first_frame_sent ? &crc_value : NULL);
    
    for (auto i = 0; i < data.size; ++i)
    {
        ROTU_PUSH_DATA(data.data[i]);
    }
    ROTU_total_data += data.size;

    if (!is_first_frame_sent)
    {
        is_first_frame_sent = true;
    }

    return result::Result::OK;
}

result::Result fs_status(FileSystemInterface::FSStatus& status)
{
    if (!is_fs_communication_valid())
    {
        return result::Result::ERROR_GENERAL;
    }

    ble::CommandToMemoryQueueElement cmd{ble::CommandToMemory::GET_FS_STATUS, 0};
    ble::StatusFromMemoryQueueElement response;

    const auto cmd_result = xQueueSend(_command_to_fs_queue, &cmd, 0);
    if (pdTRUE != cmd_result)
    {
        NRF_LOG_ERROR("fs stat: failed to send cmd to mem");
        return result::Result::ERROR_GENERAL;
    }
    const auto status_result = xQueueReceive(_status_from_fs_queue, &response, max_status_wait_time);
    if (pdTRUE != status_result)
    {
        NRF_LOG_ERROR("fs stat: timed out recv status from mem");
        return result::Result::ERROR_GENERAL; 
    }
    if (response.status != ble::StatusFromMemory::OK)
    {
        NRF_LOG_ERROR("fs stat: recv error status(%d)", static_cast<int>(response.status));
        return result::Result::ERROR_GENERAL;
    }
    
    ble::FileDataFromMemoryQueueElement& data{data_from_memory_queue_element};

    const auto data_result = xQueueReceive(_data_from_fs_queue, &data, max_short_data_wait_time);
    if (pdTRUE != data_result)
    {
        NRF_LOG_ERROR("fs stat: timed out recv data from mem");
        return result::Result::ERROR_GENERAL;
    }
    
    if (data.size != sizeof(status))
    {
        NRF_LOG_ERROR("fs stat: struct size mismatch")
    }

    memcpy(&status, data.data, sizeof(status));
    return result::Result::OK;
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
