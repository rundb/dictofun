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

constexpr uint32_t file_0_size{512};
constexpr uint32_t file_0_frequency{16000};
constexpr uint32_t file_0_codec{0};

struct TestContext
{
    file_id_type current_file_id{0ULL};
    bool is_file_open{false};
    uint32_t position{0};
    uint32_t size{0};
} _test_ctx;

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
        uint32_t idx{0};
        idx += snprintf(&tmp[idx], max_buffer_size - idx - 1, "{\"s\":%lu,\"f\":%lu,\"c\":%lu}", file_0_size, file_0_frequency, file_0_codec);

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

result::Result dictofun_test_close_file(file_id_type file_id)
{
    if (_test_ctx.is_file_open) 
    {
        return result::Result::ERROR_BUSY;
    }
    _test_ctx.is_file_open = false;
    _test_ctx.position = 0;
    _test_ctx.current_file_id = 0;

    return result::Result::OK;
}

result::Result dictofun_test_open_file(file_id_type file_id, uint32_t& file_size)
{
    if (_test_ctx.is_file_open) 
    {
        return result::Result::ERROR_BUSY;
    }

    if (file_id == test_files_ids[0])
    {
        _test_ctx.is_file_open = true;
        _test_ctx.current_file_id = file_id;
        _test_ctx.position = 0;
        _test_ctx.size = file_0_size;
        return result::Result::OK;
    }
    else if (file_id == test_files_ids[1])
    {
        return result::Result::ERROR_NOT_IMPLEMENTED;
    }
    else
    {
        return result::Result::ERROR_INVALID_PARAMETER;
    }
}

result::Result dictofun_test_get_data(file_id_type file_id, uint8_t * buffer, uint32_t& actual_size, uint32_t max_size)
{
    if (!_test_ctx.is_file_open || file_id != _test_ctx.current_file_id || buffer == nullptr)
    {
        return result::Result::ERROR_INVALID_PARAMETER;
    }
    actual_size = std::min(max_size, _test_ctx.size - _test_ctx.position);
    for (uint32_t i = _test_ctx.position; i < _test_ctx.position + actual_size; ++i)
    {
        buffer[i - _test_ctx.position] = static_cast<uint8_t>(i % 0xFF);
    }
    return result::Result::OK;
}

FileSystemInterface dictofun_test_fs_if
{
    dictofun_test_get_file_list,
    dictofun_test_get_file_info,
    dictofun_test_open_file,
    dictofun_test_close_file,
    dictofun_test_get_data,
};

}

namespace target
{

}
}
