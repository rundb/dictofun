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
result::Result get_files_list(lfs_t& lfs, uint32_t& count, uint8_t * buffer, uint32_t max_data_size);
}
}