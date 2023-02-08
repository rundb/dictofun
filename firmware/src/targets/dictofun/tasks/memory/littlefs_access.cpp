// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */
#include "littlefs_access.h"
#include "nrf_log.h"

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

result::Result get_files_list(lfs_t& lfs, uint32_t& count, uint8_t * buffer, const uint32_t max_data_size)
{
    lfs_dir_t dir;
    lfs_info info;
    const auto dir_open_result = lfs_dir_open(&lfs, &dir, ".");
    if (dir_open_result != 0)
    {
        NRF_LOG_ERROR("mem: dir open failed (%d)", dir_open_result);
        return result::Result::ERROR_GENERAL;
    }
    auto dir_read_result = lfs_dir_read(&lfs, &dir, &info);
    if (dir_read_result < 0)
    {
        NRF_LOG_ERROR("mem: dir read failed (%d)", dir_read_result);
        return result::Result::ERROR_GENERAL;
    }
    NRF_LOG_INFO("dir: %d %d %d %d", dir_read_result, dir.id, dir.type, dir.pos);
    NRF_LOG_INFO("info: %d %d %s", info.type, info.size, info.name);
    lfs_soff_t offset = lfs_dir_tell(&lfs, &dir);
    const auto seek_result = lfs_dir_seek(&lfs, &dir, offset);
    if (seek_result < 0)
    {
        NRF_LOG_ERROR("mem: dir seek failed (%d)", dir_read_result);
        return result::Result::ERROR_GENERAL;
    }
    dir_read_result = lfs_dir_read(&lfs, &dir, &info);
    if (dir_read_result < 0)
    {
        NRF_LOG_ERROR("mem: dir read failed (%d)", dir_read_result);
        return result::Result::ERROR_GENERAL;
    }
    NRF_LOG_INFO("dir: %d %d %d %d", dir_read_result, dir.id, dir.type, dir.pos);
    NRF_LOG_INFO("info: %d %d %s", info.type, info.size, info.name);
    return result::Result::ERROR_NOT_IMPLEMENTED;
}

}
}