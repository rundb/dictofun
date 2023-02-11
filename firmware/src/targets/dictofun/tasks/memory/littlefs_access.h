// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */
#pragma once
#include "result.h"
#include <stdint.h>
#include "lfs.h"

namespace memory
{
namespace filesystem
{

result::Result init_littlefs(lfs_t& lfs, const lfs_config& config);
result::Result get_files_list(lfs_t& lfs, uint32_t& data_size_bytes, uint8_t * buffer, uint32_t max_data_size);
result::Result get_file_info(lfs_t& lfs, const char * name, uint8_t * buffer, uint32_t& data_size_bytes, uint32_t max_data_size);
result::Result open_file(lfs_t& lfs, const char * name, uint32_t& file_size_bytes);
result::Result close_file(lfs_t& lfs, const char * name);
// should be used after `open()` call has been executed
result::Result get_file_data(lfs_t& lfs, uint8_t * buffer, uint32_t& actual_size, uint32_t max_data_size);
result::Result get_fs_stat(lfs_t& lfs, uint8_t * buffer, const lfs_config&  config);
}
}
