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

const struct lfs_config lfs_configuration = {
    nullptr,
    memory::block_device::read,
    memory::block_device::program,
    memory::block_device::erase,
    memory::block_device::sync,

    // block device configuration
    16,
    BLOCK_SIZE,
    SECTOR_SIZE,
    4096,
     500,
    BLOCK_SIZE,
    LOOKAHEAD_BUFFER_SIZE,
    read_buffer,
    prog_buffer,
    lookahead_buffer,
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
        const auto create_result = lfs_file_open(&lfs, &file, "rec0", LFS_O_WRONLY | LFS_O_CREAT);
        EXPECT_EQ(create_result, 0);


        constexpr size_t test_data_size{16};
        uint8_t test_data[test_data_size];
        for (int i = 0; i < test_data_size; ++i)
        {
            test_data[i] = i;
        }
        for (int i = 0; i < 20; ++i)
        {
            const auto write_result = lfs_file_write(&lfs, &file, test_data, sizeof(test_data));
            EXPECT_EQ(write_result, test_data_size);
        }

        const auto close_result_1 = lfs_file_close(&lfs, &file);
        EXPECT_EQ(close_result_1, 0);

        memset(test_data, 0, test_data_size);
        const auto open_to_read_result = lfs_file_open(&lfs, &file, "rec0", LFS_O_RDONLY);
        EXPECT_EQ(open_to_read_result, 0);
        const auto read_result = lfs_file_read(&lfs, &file, test_data, sizeof(test_data));
        EXPECT_EQ(read_result, test_data_size);
        const auto close_result_2 = lfs_file_close(&lfs, &file);
        EXPECT_EQ(close_result_2, 0);

        EXPECT_EQ(test_data[1], 1);
        EXPECT_EQ(test_data[8], 8);
        EXPECT_EQ(test_data[15], 15);
    }
}

TEST(LittleFsTest, StaticWrite)
{
    memory::block_device::sim::InRamFlash inram_flash{sim_sector_size, sim_page_size, sim_total_size};
    memory::block_device::register_flash_device(&inram_flash, sim_sector_size, sim_page_size, sim_total_size);
    
    int err = lfs_format(&lfs, &lfs_configuration);
    err = lfs_mount(&lfs, &lfs_configuration);
    EXPECT_EQ(err, 0);

    static constexpr size_t active_file_buffer_size{512};
    static uint8_t _active_file_buffer[active_file_buffer_size]{0};
    static lfs_file_t _active_file;
    static lfs_file_config _active_file_config;

    {
        memset(&_active_file_config, 0, sizeof(_active_file_config));
        _active_file_config.buffer = _active_file_buffer;
        _active_file_config.attr_count = 0;
        const auto create_result = lfs_file_opencfg(&lfs, &_active_file, "record00", LFS_O_WRONLY | LFS_O_CREAT, &_active_file_config);
        EXPECT_EQ(create_result, 0);
    }
    {
        uint8_t test_data[256];
        memset(test_data, 0xef, sizeof(test_data));
        const auto write_result = lfs_file_write(&lfs, &_active_file, test_data, sizeof(test_data));
        EXPECT_EQ(write_result, sizeof(test_data));
    }
    {
        const auto close_result = lfs_file_close(&lfs, &_active_file);
        EXPECT_EQ(close_result, 0);
    }
}

}
