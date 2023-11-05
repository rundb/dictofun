// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */
#include "myfs_access.h"
#include "ble_fts.h"
#include "nrf_log.h"
#include "myfs.h"

#include <algorithm>
#include <cstring>
#include <cstdio>

namespace memory
{
namespace filesystem
{

static ::filesystem::myfs_file_t _active_file;

static bool _is_file_open{false};
static bool _is_files_list_next_needed{false};
static constexpr uint32_t invalid_files_count{0xFEFEFEFDUL};
static uint32_t _total_files_left{0};

result::Result init_fs(::filesystem::myfs_t& fs)
{
    const ::filesystem::myfs_config& config(fs.config);
    auto err = myfs_mount(&fs, &config);
    if(err != 0)
    {
        NRF_LOG_WARNING("mem: formatting FS");
        err = myfs_format(&fs, &config);
        if(err != 0)
        {
            NRF_LOG_ERROR("mem: failed to format MyFS (%d)", err);
            return result::Result::ERROR_GENERAL;
        }
        err = myfs_mount(&fs, &config);
        if(err != 0)
        {
            NRF_LOG_ERROR("mem: failed to mount MyFS after formatting");
            return result::Result::ERROR_GENERAL;
        }
    }

    _is_files_list_next_needed = false;
    _is_file_open = false;

    return result::Result::OK;
}

result::Result deinit_fs(::filesystem::myfs_t& fs)
{
    const ::filesystem::myfs_config& config(fs.config);
    if(_active_file.is_open)
    {
        const auto close_result = close_file(fs);
        if(result::Result::OK != close_result)
        {
            NRF_LOG_ERROR("failed to close active file at deinit stage");
        }
        _is_file_open = false;
    }
    const auto unmount_result = myfs_unmount(&fs, config);
    if(unmount_result < 0)
    {
        NRF_LOG_ERROR("failed to unmount MyFS (%d)", unmount_result);
        return result::Result::ERROR_GENERAL;
    }
    return result::Result::OK;
}

result::Result close_file(::filesystem::myfs_t& fs)
{
    const ::filesystem::myfs_config& config(fs.config);
    // TODO: add comparison of the name to the name of an active file
    const auto close_result = myfs_file_close(&fs, config, _active_file);
    if(close_result < 0)
    {
        NRF_LOG_ERROR("failed to close active file");
        return result::Result::ERROR_GENERAL;
    }
    // todo: consider removing this var, as it exists within the _active_file
    _is_file_open = false;
    return result::Result::OK;
}


result::Result get_files_list(::filesystem::myfs_t& fs,
                              uint32_t& total_data_size_bytes,
                              uint8_t* buffer,
                              const uint32_t max_data_size)
{
    const ::filesystem::myfs_config& config(fs.config);
    // TODO: add an assert if max_data_size % single_entry_size != 0
    if(buffer == nullptr || max_data_size < 8)
    {
        return result::Result::ERROR_INVALID_PARAMETER;
    }
    _is_files_list_next_needed = false;

    // First perform a dry run, to get the idea of how many files we've got in the FS
    const auto total_files_count = myfs_get_files_count(fs);

    static constexpr uint32_t single_entry_size{sizeof(ble::fts::file_id_type)};
    
    static const uint32_t max_files_fitting_in_buffer{(max_data_size - 8) / single_entry_size};
    uint8_t buffer_pos{0};
    uint32_t file_ids_count{0};

    myfs_rewind_dir(fs);
    uint8_t file_id_buffer[::filesystem::myfs_file_t::id_size + 1]{0};
    char file_name_buffer[single_entry_size + 1]{0};

    // TODO: check off-by-1 chance here (if there is -1 file ID in the list)
    while(file_ids_count < max_files_fitting_in_buffer)
    {
        const auto dir_read_res = myfs_get_next_id(fs, config, file_id_buffer);
        if(dir_read_res < 0)
        {
            NRF_LOG_ERROR("dir read operation failed (%d)", dir_read_res);
            return result::Result::ERROR_GENERAL;
        }

        if(dir_read_res == 0)
        {
            break;
        }

        convert_myfs_id_to_filename(file_id_buffer, file_name_buffer);
        memcpy(&buffer[buffer_pos], file_name_buffer, single_entry_size);
        ++file_ids_count;
        buffer_pos += single_entry_size;
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
get_files_list_next(::filesystem::myfs_t& fs,
                    uint32_t& data_size_bytes,
                    uint8_t* buffer,
                    const uint32_t max_data_size)
{
    if(!_is_files_list_next_needed || buffer == nullptr)
    {
        return result::Result::ERROR_GENERAL;
    }
    const ::filesystem::myfs_config& config(fs.config);

    static constexpr uint32_t single_entry_size{sizeof(ble::fts::file_id_type)};
    static const uint32_t max_files_fitting_in_buffer{max_data_size / single_entry_size};

    uint8_t buffer_pos{0};
    uint32_t file_ids_count{0};
    uint8_t file_id_buffer[::filesystem::myfs_file_t::id_size + 1]{0};
    char file_name_buffer[single_entry_size + 1]{0};

    // TODO: check off-by-1 chance here (if there is -1 file ID in the list)
    while(file_ids_count < max_files_fitting_in_buffer)
    {
        const auto dir_read_res = myfs_get_next_id(fs, config, file_id_buffer);
        if(dir_read_res < 0)
        {
            NRF_LOG_ERROR("dir read operation failed (%d)", dir_read_res);
            return result::Result::ERROR_GENERAL;
        }

        if(dir_read_res == 0)
        {
            break;
        }

        convert_myfs_id_to_filename(file_id_buffer, file_name_buffer);
        memcpy(&buffer[buffer_pos], file_name_buffer, single_entry_size);
        ++file_ids_count;
        buffer_pos += single_entry_size;
    }

    if(file_ids_count < _total_files_left)
    {
        _is_files_list_next_needed = true;
        _total_files_left -= file_ids_count;
    }

    data_size_bytes = buffer_pos;
    return result::Result::OK;
}

result::Result get_file_info(::filesystem::myfs_t& fs, 
                             const char* name,
                             uint8_t* buffer,
                             uint32_t& data_size_bytes,
                             uint32_t max_data_size)
{
    if(buffer == nullptr || max_data_size < 8)
    {
        return result::Result::ERROR_INVALID_PARAMETER;
    }
    uint8_t id[::filesystem::myfs_file_t::id_size]{0};
    const ::filesystem::myfs_config& config(fs.config);
    
    convert_filename_to_myfs_id(name, id);
    const auto size = myfs_file_get_size(fs, config, id);
    if (size < 0)
    {
        return result::Result::ERROR_GENERAL;
    }
    // At this point we know the file size - info.size
    data_size_bytes =
        snprintf(reinterpret_cast<char*>(buffer), max_data_size, "{\"s\":%lu}", static_cast<uint32_t>(size));
    buffer[data_size_bytes] = 0;
    
    return result::Result::OK;
}

result::Result open_file(::filesystem::myfs_t& fs, const char* name, uint32_t& file_size_bytes)
{
    if (nullptr == name)
    {
        return result::Result::ERROR_GENERAL;
    }
    if (_active_file.is_open || fs.is_file_open)
    {
        return result::Result::ERROR_GENERAL;
    }

    const ::filesystem::myfs_config& config(fs.config);

    uint8_t id[::filesystem::myfs_file_t::id_size]{0};
    convert_filename_to_myfs_id(name, id);
    const auto get_size_result = myfs_file_get_size(fs, config, id);
    if (get_size_result < 0)
    {
        NRF_LOG_ERROR("fs integr: failed to get file size before open file %s", name);
        return result::Result::ERROR_GENERAL;
    }

    const auto open_result = myfs_file_open(&fs, config, _active_file, id, ::filesystem::MYFS_READ_FLAG);
    if (open_result < 0)
    {
        NRF_LOG_ERROR("fs integr: failed to open file %s", name);
        return result::Result::ERROR_GENERAL;
    }
    file_size_bytes = get_size_result;
    
    _is_file_open = true;
    return result::Result::OK;
}

result::Result
get_file_data(::filesystem::myfs_t& fs, uint8_t* buffer, uint32_t& actual_size, uint32_t max_data_size)
{
    if(buffer == nullptr || max_data_size == 0)
    {
        return result::Result::ERROR_INVALID_PARAMETER;
    }
    if (!fs.is_file_open || _active_file.is_write)
    {
        NRF_LOG_ERROR("fs integr: file is not correctly open");
        return result::Result::ERROR_GENERAL;
    }

    const ::filesystem::myfs_config& config(fs.config);

    const auto read_result = myfs_file_read(&fs, config, _active_file, buffer, max_data_size, actual_size);
    if(read_result < 0)
    {
        NRF_LOG_ERROR("read err(%d)", read_result);
        return result::Result::ERROR_GENERAL;
    }
    
    return result::Result::OK;
}

result::Result get_fs_stat(::filesystem::myfs_t& fs, uint8_t* buffer)
{
    if(buffer == nullptr)
    {
        return result::Result::ERROR_INVALID_PARAMETER;
    }

    const ::filesystem::myfs_config& config(fs.config);

    ble::fts::FileSystemInterface::FSStatus fs_stat;
    uint32_t files_count{0};
    uint32_t occupied_space{0};
    const auto get_stat_result = myfs_get_fs_stat(fs, config, files_count, occupied_space);

    if (get_stat_result < 0)
    {
        return result::Result::ERROR_GENERAL;
    }
    fs_stat.files_count = files_count;
    fs_stat.occupied_space = occupied_space;
    fs_stat.free_space = config.block_count * 4096 - occupied_space;
    memcpy(buffer, &fs_stat, sizeof(fs_stat));
    
    return result::Result::OK;
}

result::Result create_file(::filesystem::myfs_t& fs, uint8_t* file_id)
{
    if (nullptr == file_id)
    {
        return result::Result::ERROR_GENERAL;
    }
    const ::filesystem::myfs_config& config(fs.config);
    const auto create_res = myfs_file_open(&fs, config, _active_file, file_id, ::filesystem::MYFS_CREATE_FLAG);

    if(create_res < 0)
    {
        NRF_LOG_ERROR("failed to create file");
        return result::Result::ERROR_GENERAL;
    }
    _is_file_open = true;
    return result::Result::OK;
}

result::Result write_data(::filesystem::myfs_t& fs, uint8_t* data, uint32_t data_size)
{
    const ::filesystem::myfs_config& config(fs.config);
    if (nullptr == data)
    {
        return result::Result::ERROR_GENERAL;
    }
    const auto write_result = myfs_file_write(&fs, config, _active_file, reinterpret_cast<void *>(data), data_size);
    if(write_result < 0)
    {
        NRF_LOG_ERROR("failed to write data to the active file");
        return result::Result::ERROR_GENERAL;
    }

    return result::Result::OK;
}

void convert_filename_to_myfs_id(const char* name, uint8_t * file_id)
{
    if (nullptr == file_id || nullptr == name)
    {
        return;
    }
    for (auto i = 0; i < ::filesystem::myfs_file_t::id_size; ++i)
    {
        char tmp[3];
        tmp[0] = name[2 * i];
        tmp[1] = name[2 * i + 1];
        tmp[2] = '\0';
        file_id[i] = static_cast<uint8_t>(strtol(tmp, nullptr, 10));
    }
}

void convert_myfs_id_to_filename(const uint8_t * file_id, char* name)
{
    if (nullptr == file_id || nullptr == name)
    {
        return;
    }

    int pos{0};
    static constexpr auto id_size = sizeof(ble::fts::file_id_type) + 1;
    for (auto i = 0; i < ::filesystem::myfs_file_t::id_size; ++i)
    {
        pos += snprintf(&name[pos], id_size - pos, "%02d", file_id[i]);
    }
}


} // namespace filesystem
} // namespace memory
