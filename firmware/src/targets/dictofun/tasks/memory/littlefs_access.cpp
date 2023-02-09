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

}
}