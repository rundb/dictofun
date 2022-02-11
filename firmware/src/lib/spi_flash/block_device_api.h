// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2021, Roman Turkin
 */

#pragma once

#include "spi_flash.h"
#include <simple_fs.h>

namespace integration
{

static const size_t MEMORY_VOLUME = 0x800000UL;
void init_filesystem(flash::SpiFlash * spiFlash);
extern const filesystem::SpiFlashConfiguration spi_flash_simple_fs_config;
}
