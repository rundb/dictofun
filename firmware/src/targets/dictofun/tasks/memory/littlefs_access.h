// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */
#pragma once
#include "lfs.h"
#include "result.h"
#include <stdint.h>

#include "myfs.h"

namespace memory
{
namespace filesystem
{

result::Result init_fs(::filesystem::myfs_t& fs, const ::filesystem::myfs_config& config);
result::Result deinit_fs(::filesystem::myfs_t& fs, const ::filesystem::myfs_config& config);
result::Result close_file(::filesystem::myfs_t& fs, const ::filesystem::myfs_config& config);

// Following methods face into the BLE part of the system
result::Result get_files_list(::filesystem::myfs_t& fs,
                              const ::filesystem::myfs_config& config,
                              uint32_t& total_data_size_bytes,
                              uint8_t* buffer,
                              uint32_t max_data_size);
result::Result get_files_list_next(::filesystem::myfs_t& fs, 
                              const ::filesystem::myfs_config& config,
                              uint32_t& data_size_bytes, 
                              uint8_t* buffer, 
                              uint32_t max_data_size);
result::Result get_file_info(::filesystem::myfs_t& fs, 
                             const ::filesystem::myfs_config& config,
                             const char* name,
                             uint8_t* buffer,
                             uint32_t& data_size_bytes,
                             uint32_t max_data_size);
result::Result open_file(::filesystem::myfs_t& fs, const ::filesystem::myfs_config& config, const char* name, uint32_t& file_size_bytes);
result::Result get_file_data(::filesystem::myfs_t& fs, const ::filesystem::myfs_config& config, uint8_t* buffer, uint32_t& actual_size, uint32_t max_data_size);
result::Result get_fs_stat(lfs_t& lfs, uint8_t* buffer, const lfs_config& config);

// Following methods face into audio part of the system
result::Result create_file(::filesystem::myfs_t& fs, const ::filesystem::myfs_config& config, uint8_t* file_id);
result::Result write_data(::filesystem::myfs_t& fs, const ::filesystem::myfs_config& config, uint8_t* data, uint32_t data_size);

void convert_filename_to_myfs_id(const char* name, uint8_t * file_id);
void convert_myfs_id_to_filename(const uint8_t * file_id, char* name);

} // namespace filesystem
} // namespace memory
