// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#pragma once

#include "stdint.h"

namespace time
{

struct DateTime
{
    uint32_t year{0U};
    uint8_t month{0U};
    uint8_t day{0U};
    uint8_t weekday{0U};
    uint8_t hour{0U};
    uint8_t minute{0U};
    uint8_t second{0U};
};

} // namespace time
