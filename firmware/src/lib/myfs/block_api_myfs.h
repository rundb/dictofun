// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#pragma once

#include "myfs.h"
#include "spi_flash_if.h"

namespace memory
{
namespace block_device
{

void myfs_register_flash_device(memory::SpiNorFlashIf* flash,
                                uint32_t sector_size,
                                uint32_t page_size,
                                uint32_t memory_size);

int myfs_read(const struct ::filesystem::myfs_config* c,
              myfs_block_t block,
              myfs_off_t off,
              void* buffer,
              myfs_size_t size);
int myfs_program(const struct ::filesystem::myfs_config* c,
                 myfs_block_t block,
                 myfs_off_t off,
                 const void* buffer,
                 myfs_size_t size);
int myfs_erase(const struct ::filesystem::myfs_config* c, myfs_block_t block);
int myfs_sync(const struct ::filesystem::myfs_config* c);

} // namespace block_device
} // namespace memory
