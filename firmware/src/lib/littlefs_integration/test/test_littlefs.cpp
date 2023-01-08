// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "spi_flash_simulation_api.h"

#include <gtest/gtest.h>

#include "lfs.h"

namespace
{

static const uint32_t BLOCK_SIZE = 256;
static const uint32_t SECTOR_SIZE = 4096;

static const size_t CACHE_SIZE = 2 * BLOCK_SIZE;
static const size_t LOOKAHEAD_BUFFER_SIZE = 16;
static uint8_t prog_buffer[CACHE_SIZE];
static uint8_t read_buffer[CACHE_SIZE];
static uint8_t lookahead_buffer[CACHE_SIZE];

// configuration of the filesystem is provided by this struct
const struct lfs_config lfs_configuration = {
    // block device operations
    .read  = memory::blockdevice::sim::read,
    .prog  = memory::blockdevice::sim::program,
    .erase = memory::blockdevice::sim::erase,
    .sync  = memory::blockdevice::sim::sync,

    // block device configuration
    .read_size = 16,
    .prog_size = BLOCK_SIZE,
    .block_size = SECTOR_SIZE,
    .block_count = 4096,
    .block_cycles = 500,
    .cache_size = BLOCK_SIZE,
    .lookahead_size = LOOKAHEAD_BUFFER_SIZE,
    .read_buffer = read_buffer,
    .prog_buffer = prog_buffer,
    .lookahead_buffer = lookahead_buffer,
};

TEST(LittleFsTest, Dummy)
{

}

}