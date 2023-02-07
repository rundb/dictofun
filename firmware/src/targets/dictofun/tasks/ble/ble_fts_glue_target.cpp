// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "ble_fts_glue.h"
#include <cstdio>
#include <algorithm>
#include "ble_fts.h"

using namespace ble::fts;

namespace integration
{

namespace target
{

result::Result get_file_list(uint32_t& files_count, file_id_type * files_list_ptr)
{
    return result::Result::ERROR_NOT_IMPLEMENTED;
}

result::Result get_file_info(file_id_type file_id, uint8_t * file_data, uint32_t& file_data_size, const uint32_t max_file_data_size)
{
    return result::Result::ERROR_NOT_IMPLEMENTED;
}

result::Result open_file(file_id_type file_id, uint32_t& file_size)
{
    return result::Result::ERROR_NOT_IMPLEMENTED;
}

result::Result close_file(file_id_type file_id)
{
    return result::Result::ERROR_NOT_IMPLEMENTED;
}

result::Result get_data(file_id_type file_id, uint8_t * buffer, uint32_t& actual_size, uint32_t max_size)
{
    return result::Result::ERROR_NOT_IMPLEMENTED;
}

result::Result fs_status(FileSystemInterface::FSStatus& status)
{
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
