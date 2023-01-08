// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "spi_flash_simulation_api.h"
#include "block_api.h"

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
    .read  = memory::block_device::read,
    .prog  = memory::block_device::program,
    .erase = memory::block_device::erase,
    .sync  = memory::block_device::sync,

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

lfs_t lfs;
lfs_file_t file;

constexpr size_t sim_sector_size{4096};
constexpr size_t sim_page_size{256};
constexpr size_t sim_total_size{16*1024*1024};

TEST(LittleFsTest, BasicFormat)
{
    memory::block_device::sim::InRamFlash inram_flash{sim_sector_size, sim_page_size, sim_total_size};
    memory::block_device::register_flash_device(&inram_flash, sim_sector_size, sim_page_size, sim_total_size);
    
    int err = lfs_format(&lfs, &lfs_configuration);
    err = lfs_mount(&lfs, &lfs_configuration);
    EXPECT_EQ(err, 0);

    {
        lfs_file_open(&lfs, &file, "rec0", LFS_O_WRONLY | LFS_O_CREAT);
        constexpr size_t single_frame_size{64};
        constexpr size_t target_file_size{1024*1024}; // ~1Mb
        uint8_t test_data[64];
        memset(test_data, 0xEF, sizeof(test_data));
        for (auto i = 0; i < target_file_size / single_frame_size; ++i)
        {
            lfs_file_write(&lfs, &file, test_data, sizeof(test_data));
        }
        lfs_file_close(&lfs, &file);
    }

    {
        lfs_file_open(&lfs, &file, "rec1", LFS_O_WRONLY | LFS_O_CREAT);
        constexpr size_t single_frame_size{64};
        constexpr size_t target_file_size{1024*1024}; 
        uint8_t test_data[64];
        memset(test_data, 0xEF, sizeof(test_data));
        for (auto i = 0; i < target_file_size / single_frame_size; ++i)
        {
            lfs_file_write(&lfs, &file, test_data, sizeof(test_data));
        }
        lfs_file_close(&lfs, &file);
    }
}

}