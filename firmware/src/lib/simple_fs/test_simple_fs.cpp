// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#include "simple_fs.h"
#include <iostream>
#include <stdlib.h>
#include <cstring>
#include <test_runner.h>

using namespace std;

// TODO: implement tests for simple_fs.h implementation in accordance with the interface requirements described in the header. 
namespace test 
{
extern filesystem::SpiFlashConfiguration test_spi_flash_config;
extern void initFsMock();
extern void print_memory(size_t offset, size_t size);
}

void test_single_write_single_read_within_one_sector()
{
    test::initFsMock();
    const auto init_res = filesystem::init(test::test_spi_flash_config);
    ASSERT(init_res == result::Result::OK);

    filesystem::File file;
    const auto open_write_res = filesystem::open(file, filesystem::FileMode::WRONLY);
    ASSERT(open_write_res == result::Result::OK);

    static const size_t TEST_PAGE_DATA_SIZE = 256;
    static const size_t TEST_PAGES_COUNT = 4;
    uint8_t sample_data_page[TEST_PAGE_DATA_SIZE];
    uint8_t sample_data_counter = 1;
    for (auto& c: sample_data_page)
    {
        c = sample_data_counter++;
    }

    for (size_t i = 0U; i < TEST_PAGES_COUNT; ++i)
    {
        const auto write_res = filesystem::write(file, sample_data_page, TEST_PAGE_DATA_SIZE);
        ASSERT(write_res == result::Result::OK);
    }
    const auto close_res = filesystem::close(file);
    ASSERT(close_res == result::Result::OK);

    const auto open_read_res = filesystem::open(file, filesystem::FileMode::RDONLY);
    ASSERT(open_read_res == result::Result::OK);

    ASSERT(file.rom.size == TEST_PAGES_COUNT * TEST_PAGE_DATA_SIZE);
    ASSERT(file.ram.size == TEST_PAGES_COUNT * TEST_PAGE_DATA_SIZE);
    ASSERT(file.rom.magic == 0x92F61455UL);
    ASSERT(file.rom.next_file_start == 0x1000);
    for (auto& c: sample_data_page)
    {
        c = 0xEF;
    }
    size_t actual_read_size = 0;
    size_t actual_total_read_size = 0;
    do
    {
        const auto read_res = filesystem::read(file, sample_data_page, TEST_PAGE_DATA_SIZE, actual_read_size);
        ASSERT(read_res == result::Result::OK);
        actual_total_read_size += actual_read_size;
    } while (actual_read_size != 0);
    ASSERT(actual_total_read_size == file.rom.size);

}

void run_tests()
{
    TestRunner tr;
    RUN_TEST(tr, test_single_write_single_read_within_one_sector);
}

int main()
{
    run_tests();
    return 0;
}