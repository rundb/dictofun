// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */
#pragma once

#include <stdint.h>
#include <functional>
#include "result.h"

namespace ble
{
namespace fts
{

using file_id_type = uint64_t;

/// @brief This structure provides a glue to the BLE control
///        functions used in the system 
struct BleInterface
{

};

/// @brief This structure provides a glue to the file system
struct FileSystemInterface
{
    using file_list_get_function_type = std::function<result::Result (uint32_t&, file_id_type *)>;

    file_list_get_function_type file_list_get_function;
};

// TODO: consider replacing the glue structures above with a template
class FtsService
{
public:
    explicit FtsService(FileSystemInterface& fs_if, BleInterface& ble_if);
    FtsService() = delete;
    FtsService(const FtsService&) = delete;
    FtsService(FtsService&&) = delete;
    FtsService& operator=(const FtsService&) = delete;
    FtsService& operator=(FtsService&&) = delete;
    ~FtsService() = default;
private:
    FileSystemInterface& _fs_if;
    BleInterface& _ble_if;
};

}
}