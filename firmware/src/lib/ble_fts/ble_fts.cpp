// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "ble_fts.h"

namespace ble
{
namespace fts
{

FtsService::FtsService(FileSystemInterface& fs_if, BleInterface& ble_if)
: _fs_if(fs_if)
, _ble_if(ble_if)
{
}

}
}