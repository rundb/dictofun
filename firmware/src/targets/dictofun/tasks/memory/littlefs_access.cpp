// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */
#include "littlefs_access.h"
#include "ble_fts.h"
#include "nrf_log.h"
#include <algorithm>
#include <cstring>

namespace memory
{
namespace filesystem
{
// This is a workaround to keep state of the active file between the calls
// see https://github.com/littlefs-project/littlefs/issues/304 for reference
static constexpr size_t active_file_buffer_size{256};
static uint8_t _active_file_buffer[active_file_buffer_size]{0};
static lfs_file_t _active_file;
static lfs_dir_t _active_dir;
static lfs_file_config _active_file_config;
static bool _is_file_open{false};
static bool _is_files_list_next_needed{false};
static constexpr uint32_t invalid_files_count{0xFEFEFEFDUL};
static uint32_t _total_files_left{0};

static uint32_t get_files_count(lfs_t& lfs);

result::Result init_littlefs(lfs_t& lfs, const lfs_config& config)
{
    auto err = lfs_mount(&lfs, &config);
    if(err != 0)
    {
        NRF_LOG_WARNING("mem: formatting FS");
        err = lfs_format(&lfs, &config);
        if(err != 0)
        {
            NRF_LOG_ERROR("mem: failed to format LFS (%d)", err);
            return result::Result::ERROR_GENERAL;
        }
        err = lfs_mount(&lfs, &config);
        if(err != 0)
        {
            NRF_LOG_ERROR("mem: failed to mount LFS after formatting");
            return result::Result::ERROR_GENERAL;
        }
    }

    const auto dir_open_result = lfs_dir_open(&lfs, &_active_dir, ".");
    if(dir_open_result != 0)
    {
        NRF_LOG_ERROR("failed to open dir ./");
        return result::Result::ERROR_GENERAL;
    }
    _is_files_list_next_needed = false;
    _is_file_open = false;

    return result::Result::OK;
}

result::Result deinit_littlefs(lfs_t& lfs)
{
    if(_is_file_open)
    {
        const auto close_result = close_file(lfs, nullptr);
        if(result::Result::OK != close_result)
        {
            NRF_LOG_ERROR("failed to close active file at deinit stage");
        }
        _is_file_open = false;
    }
    const auto unmount_result = lfs_unmount(&lfs);
    if(unmount_result < 0)
    {
        NRF_LOG_ERROR("failed to unmount LFS (%d)", unmount_result);
        return result::Result::ERROR_GENERAL;
    }
    return result::Result::OK;
}

result::Result get_files_list(lfs_t& lfs,
                              uint32_t& total_data_size_bytes,
                              uint8_t* buffer,
                              const uint32_t max_data_size)
{
    // TODO: add an assert if max_data_size % single_entry_size != 0
    if(buffer == nullptr || max_data_size < 8)
    {
        return result::Result::ERROR_INVALID_PARAMETER;
    }
    _is_files_list_next_needed = false;

    // First perform a dry run, to get the idea of how many files we've got in the FS
    const auto total_files_count = get_files_count(lfs);
    if(total_files_count == invalid_files_count)
    {
        return result::Result::ERROR_GENERAL;
    }

    const auto rewind_result = lfs_dir_rewind(&lfs, &_active_dir);
    if(rewind_result < 0)
    {
        NRF_LOG_ERROR("rewind has failed (%d)", rewind_result);
        return result::Result::ERROR_GENERAL;
    }

    lfs_info info;
    static constexpr uint32_t single_entry_size{sizeof(ble::fts::file_id_type)};
    // FIXME: need to properly think of how to organize buffers
    static const uint32_t max_files_fitting_in_buffer{(max_data_size - 8) / single_entry_size};
    uint8_t buffer_pos{0};
    uint32_t file_ids_count{0};

    // TODO: check off-by-1 chance here (if there is -1 file ID in the list)
    while(file_ids_count < max_files_fitting_in_buffer)
    {
        const auto dir_read_res = lfs_dir_read(&lfs, &_active_dir, &info);
        if(dir_read_res < 0)
        {
            NRF_LOG_ERROR("dir read operation failed (%d)", dir_read_res);
            return result::Result::ERROR_GENERAL;
        }

        if(dir_read_res == 0)
        {
            break;
        }

        switch(info.type)
        {
        case LFS_TYPE_REG: {
            const uint32_t name_len = strlen(info.name);
            memcpy(&buffer[buffer_pos], info.name, std::min(name_len, single_entry_size));
            if(name_len < single_entry_size)
            {
                memset(&buffer[buffer_pos + name_len], 0, single_entry_size - name_len);
            }
            buffer_pos += single_entry_size;
            ++file_ids_count;
            break;
        }
        default: {
            break;
        }
        }
    }

    NRF_LOG_DEBUG("req files list: ids %d, max %d", file_ids_count, total_files_count);
    if(file_ids_count < total_files_count)
    {
        _is_files_list_next_needed = true;
        _total_files_left = total_files_count - file_ids_count;
    }

    total_data_size_bytes = total_files_count * single_entry_size;
    return result::Result::OK;
}

result::Result
get_files_list_next(lfs_t& lfs, uint32_t& data_size_bytes, uint8_t* buffer, uint32_t max_data_size)
{
    if(!_is_files_list_next_needed || buffer == nullptr)
    {
        return result::Result::ERROR_GENERAL;
    }

    lfs_info info;
    static constexpr uint32_t single_entry_size{sizeof(ble::fts::file_id_type)};
    static const uint32_t max_files_fitting_in_buffer{max_data_size / single_entry_size};

    uint8_t buffer_pos{0};
    uint32_t file_ids_count{0};

    // TODO: check off-by-1 chance here (if there is -1 file ID in the list)
    while(file_ids_count < max_files_fitting_in_buffer)
    {
        const auto dir_read_res = lfs_dir_read(&lfs, &_active_dir, &info);
        if(dir_read_res < 0)
        {
            NRF_LOG_ERROR("dir read operation failed (%d)", dir_read_res);
            return result::Result::ERROR_GENERAL;
        }

        if(dir_read_res == 0)
        {
            break;
        }

        switch(info.type)
        {
        case LFS_TYPE_REG: {
            const uint32_t name_len = strlen(info.name);
            memcpy(&buffer[buffer_pos], info.name, std::min(name_len, single_entry_size));
            if(name_len < single_entry_size)
            {
                memset(&buffer[buffer_pos + name_len], 0, single_entry_size - name_len);
            }
            buffer_pos += single_entry_size;
            ++file_ids_count;
            break;
        }
        default: {
            break;
        }
        }
    }

    if(file_ids_count < _total_files_left)
    {
        _is_files_list_next_needed = true;
        _total_files_left -= file_ids_count;
    }

    data_size_bytes = buffer_pos;
    return result::Result::OK;
}

result::Result get_file_info(lfs_t& lfs,
                             const char* name,
                             uint8_t* buffer,
                             uint32_t& data_size_bytes,
                             uint32_t max_data_size)
{
    if(buffer == nullptr || max_data_size < 8)
    {
        return result::Result::ERROR_INVALID_PARAMETER;
    }

    lfs_info info;

    const auto rewind_result = lfs_dir_rewind(&lfs, &_active_dir);
    if(rewind_result < 0)
    {
        NRF_LOG_ERROR("rewind has failed (%d)", rewind_result);
        return result::Result::ERROR_GENERAL;
    }

    const auto stat_result = lfs_stat(&lfs, name, &info);
    if(stat_result < 0)
    {
        NRF_LOG_ERROR("lfs stat error(%d)", stat_result);
        data_size_bytes = 0;
        return result::Result::OK;
    }
    if(info.type != LFS_TYPE_REG)
    {
        // File not found use-case
        data_size_bytes = 0;
        return result::Result::OK;
    }
    // At this point we know the file size - info.size
    data_size_bytes =
        snprintf(reinterpret_cast<char*>(buffer), max_data_size, "{\"s\":%lu}", info.size);

    return result::Result::OK;
}

result::Result open_file(lfs_t& lfs, const char* name, uint32_t& file_size_bytes)
{
    lfs_info info;
    const auto stat_result = lfs_stat(&lfs, name, &info);
    if(stat_result < 0)
    {
        NRF_LOG_ERROR("open: lfs stat error(%d)", stat_result);
        file_size_bytes = 0;
        return result::Result::OK;
    }
    if(info.type != LFS_TYPE_REG)
    {
        // File not found use-case
        file_size_bytes = 0;
        return result::Result::ERROR_INVALID_PARAMETER;
    }
    file_size_bytes = info.size;
    memset(&_active_file_config, 0, sizeof(_active_file_config));
    _active_file_config.buffer = _active_file_buffer;
    _active_file_config.attr_count = 0;
    const auto config_res =
        lfs_file_opencfg(&lfs, &_active_file, name, LFS_O_RDONLY, &_active_file_config);

    if(config_res < 0)
    {
        NRF_LOG_ERROR("failed to open file %s", name);
        return result::Result::ERROR_GENERAL;
    }
    _is_file_open = true;
    return result::Result::OK;
}

result::Result
get_file_data(lfs_t& lfs, uint8_t* buffer, uint32_t& actual_size, const uint32_t max_data_size)
{
    if(buffer == nullptr || max_data_size == 0)
    {
        return result::Result::ERROR_INVALID_PARAMETER;
    }
    const auto result = lfs_file_read(&lfs, &_active_file, buffer, max_data_size);
    if(result < 0)
    {
        NRF_LOG_ERROR("read err(%d)", result);
        return result::Result::ERROR_GENERAL;
    }
    actual_size = result;
    return result::Result::OK;
}

result::Result close_file(lfs_t& lfs, const char* name)
{
    // TODO: add comparison of the name to the name of an active file
    const auto close_result = lfs_file_close(&lfs, &_active_file);
    if(close_result < 0)
    {
        NRF_LOG_ERROR("failed to close active file");
        return result::Result::ERROR_GENERAL;
    }
    _is_file_open = false;
    return result::Result::OK;
}

result::Result get_fs_stat(lfs_t& lfs, uint8_t* buffer, const lfs_config& config)
{
    if(buffer == nullptr)
    {
        return result::Result::ERROR_INVALID_PARAMETER;
    }
    ble::fts::FileSystemInterface::FSStatus fs_stat;
    const auto lfs_fs_size_result = lfs_fs_size(&lfs);
    if(lfs_fs_size_result < 0)
    {
        return result::Result::ERROR_GENERAL;
    }
    fs_stat.occupied_space = lfs_fs_size_result * config.block_size;
    fs_stat.free_space = config.block_size * config.block_count - fs_stat.occupied_space;

    // Count files, algorithm taken from get_files_list
    lfs_info info;

    const auto rewind_result = lfs_dir_rewind(&lfs, &_active_dir);
    if(rewind_result < 0)
    {
        NRF_LOG_ERROR("rewind has failed (%d)", rewind_result);
        return result::Result::ERROR_GENERAL;
    }

    while(true)
    {
        const auto dir_read_res = lfs_dir_read(&lfs, &_active_dir, &info);
        if(dir_read_res < 0)
        {
            NRF_LOG_ERROR("dir read operation failed (%d)", dir_read_res);
            return result::Result::ERROR_GENERAL;
        }

        if(dir_read_res == 0)
        {
            break;
        }

        switch(info.type)
        {
        case LFS_TYPE_REG: {
            ++fs_stat.files_count;
            break;
        }
        default: {
            break;
        }
        }
    }

    memcpy(buffer, &fs_stat, sizeof(fs_stat));
    return result::Result::OK;
}

result::Result get_latest_file_name(lfs_t& lfs, char* name, uint32_t& name_len)
{
    if(name == nullptr)
    {
        return result::Result::ERROR_INVALID_PARAMETER;
    }

    lfs_info info;

    const auto rewind_result = lfs_dir_rewind(&lfs, &_active_dir);
    if(rewind_result < 0)
    {
        NRF_LOG_ERROR("rewind has failed (%d)", rewind_result);
        return result::Result::ERROR_GENERAL;
    }

    static constexpr uint32_t single_entry_size{sizeof(ble::fts::file_id_type)};
    char tmp[single_entry_size + 1] = "awesome_record00";
    while(true)
    {
        const auto dir_read_res = lfs_dir_read(&lfs, &_active_dir, &info);
        if(dir_read_res < 0)
        {
            NRF_LOG_ERROR("dir read operation failed (%d)", dir_read_res);
            return result::Result::ERROR_GENERAL;
        }
        if(dir_read_res == 0)
        {
            break;
        }
        switch(info.type)
        {
        case LFS_TYPE_REG: {
            const auto cmp_result = strcmp(info.name, tmp);
            if(cmp_result > 0)
            {
                memcpy(tmp, info.name, single_entry_size);
            }
            break;
        }
        default: {
            break;
        }
        }
    }
    memcpy(name, tmp, single_entry_size);
    name_len = single_entry_size;

    return result::Result::OK;
}

result::Result create_file(lfs_t& lfs, const char* name)
{
    memset(&_active_file_config, 0, sizeof(_active_file_config));
    _active_file_config.buffer = _active_file_buffer;
    _active_file_config.attr_count = 0;
    memset(&_active_file, 0, sizeof(_active_file));
    memset(_active_file_buffer, 0, sizeof(_active_file_buffer));
    const auto config_res = lfs_file_opencfg(
        &lfs, &_active_file, name, LFS_O_CREAT | LFS_O_WRONLY, &_active_file_config);

    if(config_res < 0)
    {
        NRF_LOG_ERROR("failed to open file %s for write", name);
        return result::Result::ERROR_GENERAL;
    }
    _is_file_open = true;
    return result::Result::OK;
}

result::Result write_data(lfs_t& lfs, const uint8_t* data, const uint32_t data_size)
{
    const auto write_result = lfs_file_write(&lfs, &_active_file, data, data_size);
    if(write_result < 0)
    {
        NRF_LOG_ERROR("failed to write data to the active file");
        return result::Result::ERROR_GENERAL;
    }
    if(static_cast<uint32_t>(write_result) != data_size)
    {
        NRF_LOG_WARNING("written and target sizes mismatch (%d != %d)", write_result, data_size);
    }
    return result::Result::OK;
}

static uint32_t get_files_count(lfs_t& lfs)
{
    const auto rewind_result = lfs_dir_rewind(&lfs, &_active_dir);
    if(rewind_result < 0)
    {
        NRF_LOG_ERROR("rewind has failed (%d)", rewind_result);
        return invalid_files_count;
    }

    uint32_t files_count{0};
    lfs_info info;
    // TODO: check off-by-1 chance here (if there is -1 file ID in the list)
    while(true)
    {
        const auto dir_read_res = lfs_dir_read(&lfs, &_active_dir, &info);
        if(dir_read_res < 0)
        {
            NRF_LOG_ERROR("dir read operation failed (%d)", dir_read_res);
            return invalid_files_count;
        }

        if(dir_read_res == 0)
        {
            break;
        }

        switch(info.type)
        {
        case LFS_TYPE_REG: {
            ++files_count;
            break;
        }
        default: {
            break;
        }
        }
    }
    return files_count;
}

} // namespace filesystem
} // namespace memory
