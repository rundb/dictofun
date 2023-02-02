// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "ble_fts_glue.h"
#include <cstdio>
#include <algorithm>

using namespace ble::fts;

namespace integration
{

namespace test
{

constexpr size_t test_files_count{2};
ble::fts::file_id_type test_files_ids[test_files_count] {
    0x0102030405060708ULL,
    0x0102030405060709ULL,
};

result::Result dictofun_test_get_file_list(uint32_t& files_count, ble::fts::file_id_type * files_list_ptr)
{
    if (nullptr == files_list_ptr)
    {
        return result::Result::ERROR_INVALID_PARAMETER;
    }
    files_count = test_files_count;
    memcpy(files_list_ptr, test_files_ids, sizeof(test_files_ids));
    return result::Result::OK;
}

result::Result dictofun_test_get_file_info(const ble::fts::file_id_type file_id, uint8_t * file_data, uint32_t& file_data_size, const uint32_t max_data_size)
{
    if (file_data == nullptr)
    {
        return result::Result::ERROR_INVALID_PARAMETER;
    }

    static constexpr size_t max_buffer_size{64};
    char tmp[max_buffer_size]{0};
    if (file_id == test_files_ids[0])
    {
        // let first file have a size of 512 bytes, no codec used, frequency 16000
        // TODO: consider using string primitives from ESTL here (good chance)
        constexpr uint32_t file_size{512};
        constexpr uint32_t frequency{16000};
        constexpr uint32_t codec{0};
        uint32_t idx{0};
        idx += snprintf(&tmp[idx], max_buffer_size - idx - 1, "{\"s\":%lu,\"f\":%lu,\"c\":%lu}", file_size, frequency, codec);

        memcpy(file_data, tmp, std::min(max_data_size - 1, idx));
        file_data_size = idx;
    }
    else if (file_id == test_files_ids[1])
    {
        return result::Result::ERROR_NOT_IMPLEMENTED;
    }
    else 
    {
        return result::Result::ERROR_GENERAL;
    }
    return result::Result::OK;
}

FileSystemInterface dictofun_test_fs_if
{
    dictofun_test_get_file_list,
    dictofun_test_get_file_info,
};

}

namespace target
{

}
}
