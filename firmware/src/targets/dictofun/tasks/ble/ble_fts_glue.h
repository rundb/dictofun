// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#pragma once

#include "ble_fts.h"

namespace integration
{
extern ble::fts::BleInterface dictofun_ble_if;
namespace test
{
result::Result dictofun_test_get_file_list(uint32_t& files_count, ble::fts::file_id_type * files_list_ptr);

extern ble::fts::FileSystemInterface dictofun_test_fs_if;
}

namespace target
{

//ble::fts::FileSystemInterface dictofun_fs_if;

}
}