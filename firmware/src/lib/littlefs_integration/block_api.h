// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#pragma once

#include "lfs.h"
#include "spi_flash_if.h"

// Set of methods that implement block level API to be used in LittleFS on top of Winbond NOR SPI Flash
// Main feature introduced by this class: last sector contains bitmap of erased state for each of the sectors
// available in the flash memory.
namespace memory
{
namespace block_device
{

void register_flash_device(memory::SpiNorFlashIf* flash,
                           uint32_t sector_size,
                           uint32_t page_size,
                           uint32_t memory_size);

int read(
    const struct lfs_config* c, lfs_block_t block, lfs_off_t off, void* buffer, lfs_size_t size);
int program(const struct lfs_config* c,
            lfs_block_t block,
            lfs_off_t off,
            const void* buffer,
            lfs_size_t size);
int erase(const struct lfs_config* c, lfs_block_t block);
int sync(const struct lfs_config* c);

} // namespace block_device
} // namespace memory
