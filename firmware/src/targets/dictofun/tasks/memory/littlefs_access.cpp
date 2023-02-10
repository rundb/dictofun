// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */
#include "littlefs_access.h"
#include "nrf_log.h"
#include <cstring>
#include <algorithm>
#include "ble_fts.h"

namespace memory
{
namespace filesystem
{
// This is a workaround to keep state of the active file between the calls
// see https://github.com/littlefs-project/littlefs/issues/304 for reference
static constexpr size_t active_file_buffer_size{256};
static uint8_t _active_file_buffer[active_file_buffer_size];
static lfs_file_t _active_file;
static lfs_file_config _active_file_config;

result::Result init_littlefs(lfs_t& lfs, const lfs_config& config)
{
    auto err = lfs_mount(&lfs, &config);
    if (err != 0)
    {
        NRF_LOG_WARNING("mem: formatting FS");
        err = lfs_format(&lfs, &config);
        if (err != 0)
        {
            NRF_LOG_ERROR("mem: failed to format LFS");
            return result::Result::ERROR_GENERAL;
        }
        err = lfs_mount(&lfs, &config);
        if (err != 0)
        {
            NRF_LOG_ERROR("mem: failed to mount LFS after formatting");
            return result::Result::ERROR_GENERAL;
        }
    }
    return result::Result::OK;
}

result::Result get_files_list(lfs_t& lfs, uint32_t& data_size_bytes, uint8_t * buffer, const uint32_t max_data_size)
{
    if (buffer == nullptr || max_data_size < 8)
    {
        return result::Result::ERROR_INVALID_PARAMETER;
    }

    lfs_dir_t dir;
    lfs_info info;
    const auto dir_open_result = lfs_dir_open(&lfs, &dir, ".");
    if (dir_open_result != 0)
    {
        NRF_LOG_ERROR("mem: dir open failed (%d)", dir_open_result);
        return result::Result::ERROR_GENERAL;
    }

    static constexpr uint32_t single_entry_size{sizeof(ble::fts::file_id_type)};
    uint8_t buffer_pos{0};
    while (true) 
    {
        if (buffer_pos >= (max_data_size - single_entry_size))
        {
            NRF_LOG_WARNING("too many files in the filesystem. breaking at this point.")
            break;
        }
        const auto dir_read_res = lfs_dir_read(&lfs, &dir, &info);
        if (dir_read_res < 0) {
            NRF_LOG_ERROR("dir read operation failed (%d)", dir_read_res);
            return result::Result::ERROR_GENERAL;
        }

        if (dir_read_res == 0) {
            break;
        }

        switch (info.type) 
        {
            case LFS_TYPE_REG:
            {   
                const uint32_t name_len = strlen(info.name);
                memcpy(&buffer[buffer_pos], info.name, std::min(name_len,single_entry_size));
                if (name_len < single_entry_size)
                {
                    memset(&buffer[buffer_pos + name_len], 0, single_entry_size - name_len);
                }
                buffer_pos += single_entry_size;
                break;
            }
            default:
            {
                break;
            }
        }
    }

    data_size_bytes = buffer_pos;
    return result::Result::OK;
}

result::Result get_file_info(lfs_t& lfs, const char * name, uint8_t * buffer, uint32_t& data_size_bytes, uint32_t max_data_size)
{
    if (buffer == nullptr || max_data_size < 8)
    {
        return result::Result::ERROR_INVALID_PARAMETER;
    }

    lfs_info info;
    lfs_dir_t dir;

    const auto dir_open_result = lfs_dir_open(&lfs, &dir, ".");
    if (dir_open_result != 0)
    {
        NRF_LOG_ERROR("failed to open dir ./");
        return result::Result::ERROR_GENERAL;
    }

    const auto stat_result = lfs_stat(&lfs, name, &info);
    if (stat_result < 0)
    {
        NRF_LOG_ERROR("lfs stat error(%d)", stat_result);
        data_size_bytes = 0;
        return result::Result::OK;
    }
    if (info.type != LFS_TYPE_REG)
    {
        // File not found use-case
        data_size_bytes = 0;
        return result::Result::OK;
    }
    // At this point we know the file size - info.size
    data_size_bytes = snprintf(reinterpret_cast<char *>(buffer), max_data_size, "{\"s\":%lu}", info.size);

    return result::Result::OK;

}

result::Result open_file(lfs_t& lfs, const char * name, uint32_t& file_size_bytes)
{
    lfs_info info;
    const auto stat_result = lfs_stat(&lfs, name, &info);
    if (stat_result < 0)
    {
        NRF_LOG_ERROR("open: lfs stat error(%d)", stat_result);
        file_size_bytes = 0;
        return result::Result::OK;
    }
    if (info.type != LFS_TYPE_REG)
    {
        // File not found use-case
        file_size_bytes = 0;
        return result::Result::ERROR_INVALID_PARAMETER;
    }
    file_size_bytes = info.size;
    memset(&_active_file_config, 0, sizeof(_active_file_config));
    _active_file_config.buffer = _active_file_buffer;
    _active_file_config.attr_count = 0;
    const auto config_res = lfs_file_opencfg(&lfs, &_active_file, name, LFS_O_RDONLY, &_active_file_config);

    if (config_res < 0)
    {
        NRF_LOG_ERROR("failed to open file %s", name);
        return result::Result::ERROR_GENERAL;
    }
    return result::Result::OK;
}

result::Result get_file_data(lfs_t& lfs, uint8_t * buffer, uint32_t& actual_size, const uint32_t max_data_size)
{
    if (buffer == nullptr || max_data_size == 8)
    {
        return result::Result::ERROR_INVALID_PARAMETER;
    }
    const auto result = lfs_file_read(&lfs, &_active_file, buffer, max_data_size);
    if (result < 0)
    {
        NRF_LOG_ERROR("read err(%d)", result);
        return result::Result::ERROR_GENERAL;
    }
    actual_size = result;
    return result::Result::OK;
}

result::Result close_file(lfs_t& lfs, const char * name)
{
    const auto close_result = lfs_file_close(&lfs,&_active_file);
    if (close_result < 0)
    {
        NRF_LOG_ERROR("failed to close active file");
        return result::Result::ERROR_GENERAL;
    }
    return result::Result::OK;
}

}
}
