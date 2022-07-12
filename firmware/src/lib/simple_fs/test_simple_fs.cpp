// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */

#include "simple_fs.h"
#include <cstring>
#include <iostream>
#include <stdlib.h>
#include <test_runner.h>

using namespace std;

// TODO: implement tests for simple_fs.h implementation in accordance with the interface requirements described in the header.
namespace test
{
extern filesystem::SpiFlashConfiguration test_spi_flash_config;
extern void initFsMock();
extern void print_memory(size_t offset, size_t size);
} // namespace test

void test_init()
{
    test::initFsMock();
    const auto init_res = filesystem::init(test::test_spi_flash_config);
    ASSERT(init_res == result::Result::OK);
}

void test_single_write_single_read_within_one_sector()
{
    test::initFsMock();
    (void)filesystem::init(test::test_spi_flash_config);

    filesystem::File file;
    const auto open_write_res = filesystem::create(file);
    ASSERT(open_write_res == result::Result::OK);

    static const size_t TEST_PAGE_DATA_SIZE = 256;
    static const size_t TEST_PAGES_COUNT = 4;
    uint8_t sample_data_page[TEST_PAGE_DATA_SIZE];
    uint8_t sample_data_counter = 1;
    for(auto& c : sample_data_page)
    {
        c = sample_data_counter++;
    }

    for(size_t i = 0U; i < TEST_PAGES_COUNT; ++i)
    {
        const auto write_res = filesystem::write(file, sample_data_page, TEST_PAGE_DATA_SIZE);
        ASSERT(write_res == result::Result::OK);
    }
    const auto close_res = filesystem::close(file);
    ASSERT(close_res == result::Result::OK);

    filesystem::FilesCount file_count;
    const auto files_count_res = filesystem::get_files_count(file_count);
    ASSERT(files_count_res == result::Result::OK);
    ASSERT(file_count.invalid == 0);
    ASSERT(file_count.valid == 1);

    const auto open_read_res = filesystem::open(file);
    ASSERT(open_read_res == result::Result::OK);

    ASSERT(file.rom.size == TEST_PAGES_COUNT * TEST_PAGE_DATA_SIZE);
    ASSERT(file.ram.size == TEST_PAGES_COUNT * TEST_PAGE_DATA_SIZE);
    ASSERT(file.rom.magic == 0x92F61455UL);
    ASSERT(file.rom.next_file_start == 0x1000);
    for(auto& c : sample_data_page)
    {
        c = 0xEF;
    }
    size_t actual_read_size = 0;
    size_t actual_total_read_size = 0;
    do
    {
        const auto read_res =
            filesystem::read(file, sample_data_page, TEST_PAGE_DATA_SIZE, actual_read_size);
        ASSERT(read_res == result::Result::OK);
        actual_total_read_size += actual_read_size;
    } while(actual_read_size != 0);
    ASSERT(actual_total_read_size == file.rom.size);
    const auto close_after_read_res = filesystem::invalidate(file);
    ASSERT(close_after_read_res == result::Result::OK);
    file_count.invalid = 1000;
    file_count.valid = 1000;
    filesystem::get_files_count(file_count);
    ASSERT(file_count.invalid == 1);
    ASSERT(file_count.valid == 0);
}

void test_several_writes_before_several_reads()
{
    test::initFsMock();
    (void)filesystem::init(test::test_spi_flash_config);
    static const size_t SAMPLE_DATA_SIZE = 256;
    uint8_t sample_data[SAMPLE_DATA_SIZE];
    char sample_data_content[SAMPLE_DATA_SIZE] =
        "000:this is a sample page, at the beginning it contains a decimal "
        "number of the page being written. I want to write some text that makes no sense at all, "
        "but doesn't repeat itself. Frogs tend to show up om Wednesdays, not sure if I can confirm "
        "this trend.";
    memcpy(sample_data, sample_data_content, SAMPLE_DATA_SIZE);

    // Now write 4 files in a row
    // File 1: 0x800 bytes (8 pages)
    // File 2: 0x1000 bytes (16 pages, overlaps a sector)
    // File 3: 0x2000 bytes (32 pages, overlaps 2 sectors)
    // File 3: 0x4000 bytes (64 pages, overlaps 4 sectors)
    int page_iteration = 1;

    { // file 1
        filesystem::File file1;
        auto open_result = filesystem::create(file1);
        ASSERT(open_result == result::Result::OK);
        for(int i = 0; i < 8; ++i)
        {
            snprintf((char*)sample_data, 4, "%03d", page_iteration);
            const auto write_res = filesystem::write(file1, sample_data, SAMPLE_DATA_SIZE);
            ASSERT(write_res == result::Result::OK);
            page_iteration++;
        }

        auto close_result = filesystem::close(file1);
        ASSERT(close_result == result::Result::OK);
    }
    { // file 2
        filesystem::File file2;
        auto open_result = filesystem::create(file2);
        ASSERT(open_result == result::Result::OK);
        for(int i = 0; i < 16; ++i)
        {
            snprintf((char*)sample_data, 4, "%03d", page_iteration);
            const auto write_res = filesystem::write(file2, sample_data, SAMPLE_DATA_SIZE);
            ASSERT(write_res == result::Result::OK);
            page_iteration++;
        }

        auto close_result = filesystem::close(file2);
        ASSERT(close_result == result::Result::OK);
    }

    { // file 3
        filesystem::File file3;
        auto open_result = filesystem::create(file3);
        ASSERT(open_result == result::Result::OK);
        for(int i = 0; i < 32; ++i)
        {
            snprintf((char*)sample_data, 4, "%03d", page_iteration);
            const auto write_res = filesystem::write(file3, sample_data, SAMPLE_DATA_SIZE);
            ASSERT(write_res == result::Result::OK);
            page_iteration++;
        }

        auto close_result = filesystem::close(file3);
        ASSERT(close_result == result::Result::OK);
    }

    { // file 4
        filesystem::File file4;
        auto open_result = filesystem::create(file4);
        ASSERT(open_result == result::Result::OK);
        for(int i = 0; i < 64; ++i)
        {
            snprintf((char*)sample_data, 4, "%03d", page_iteration);
            const auto write_res = filesystem::write(file4, sample_data, SAMPLE_DATA_SIZE);
            ASSERT(write_res == result::Result::OK);
            page_iteration++;
        }

        auto close_result = filesystem::close(file4);
        ASSERT(close_result == result::Result::OK);
    }

    int read_page_id = 1;
    {
        // read file 1, verify that first 3 bytes correspond to page id for every page
        filesystem::File file1;
        const auto open_result = filesystem::open(file1);
        uint8_t read_data[SAMPLE_DATA_SIZE]{0};
        ASSERT(open_result == result::Result::OK);

        size_t read_size = 0;
        do
        {
            const auto read_res = filesystem::read(file1, read_data, SAMPLE_DATA_SIZE, read_size);
            ASSERT(read_res == result::Result::OK);
            const int current_page_id = atoi((const char*)read_data);
            if(read_size > 0)
            {
                ASSERT(read_page_id == current_page_id);
                read_page_id++;
            }
        } while(read_size > 0);
        const auto invalidation_result = filesystem::invalidate(file1);
        ASSERT(invalidation_result == result::Result::OK);
    }
    {
        // read file 2, verify that first 3 bytes correspond to page id for every page
        filesystem::File file2;
        const auto open_result = filesystem::open(file2);
        uint8_t read_data[SAMPLE_DATA_SIZE]{0};
        ASSERT(open_result == result::Result::OK);

        size_t read_size = 0;
        do
        {
            const auto read_res = filesystem::read(file2, read_data, SAMPLE_DATA_SIZE, read_size);
            ASSERT(read_res == result::Result::OK);
            const int current_page_id = atoi((const char*)read_data);
            if(read_size > 0)
            {
                ASSERT(read_page_id == current_page_id);
                read_page_id++;
            }
        } while(read_size > 0);
        const auto invalidation_result = filesystem::invalidate(file2);
        ASSERT(invalidation_result == result::Result::OK);
    }
    filesystem::FilesCount files_count;
    const auto result = filesystem::get_files_count(files_count);
    ASSERT(result == result::Result::OK);
    ASSERT(files_count.invalid == 2);
    ASSERT(files_count.valid == 2);
    const auto occupied_size = filesystem::get_occupied_memory_size();
}

void test_closing_file_during_read_without_invalidation()
{
    test::initFsMock();
    filesystem::init(test::test_spi_flash_config);

    filesystem::File file;
    const auto create_res = filesystem::create(file);
    ASSERT(create_res == result::Result::OK);

    static const size_t TEST_PAGE_DATA_SIZE = 256;
    static const size_t TEST_PAGES_COUNT = 4;
    uint8_t sample_data_page[TEST_PAGE_DATA_SIZE];
    uint8_t sample_data_counter = 1;
    for(auto& c : sample_data_page)
    {
        c = sample_data_counter++;
    }

    for(size_t i = 0U; i < TEST_PAGES_COUNT; ++i)
    {
        const auto write_res = filesystem::write(file, sample_data_page, TEST_PAGE_DATA_SIZE);
        ASSERT(write_res == result::Result::OK);
    }
    const auto close_res = filesystem::close(file);
    ASSERT(close_res == result::Result::OK);

    filesystem::FilesCount file_count;
    const auto files_count_res = filesystem::get_files_count(file_count);
    ASSERT(files_count_res == result::Result::OK);
    ASSERT(file_count.invalid == 0);
    ASSERT(file_count.valid == 1);

    const auto open_read_res = filesystem::open(file);
    ASSERT(open_read_res == result::Result::OK);

    ASSERT(file.rom.size == TEST_PAGES_COUNT * TEST_PAGE_DATA_SIZE);
    ASSERT(file.ram.size == TEST_PAGES_COUNT * TEST_PAGE_DATA_SIZE);
    ASSERT(file.rom.magic == 0x92F61455UL);
    ASSERT(file.rom.next_file_start == 0x1000);
    for(auto& c : sample_data_page)
    {
        c = 0xEF;
    }
    size_t actual_read_size = 0;
    size_t actual_total_read_size = 0;
    do
    {
        const auto read_res =
            filesystem::read(file, sample_data_page, TEST_PAGE_DATA_SIZE, actual_read_size);
        ASSERT(read_res == result::Result::OK);
        actual_total_read_size += actual_read_size;
    } while(actual_read_size != 0);

    ASSERT(actual_total_read_size == file.rom.size);

    // closing file without invalidation
    const auto close_after_read_res = filesystem::close(file);
    ASSERT(close_after_read_res == result::Result::OK);
    file_count.invalid = 1000;
    file_count.valid = 1000;
    filesystem::get_files_count(file_count);
    ASSERT(file_count.invalid == 0);
    ASSERT(file_count.valid == 1);
}

void run_tests()
{
    TestRunner tr;
    RUN_TEST(tr, test_init);
    RUN_TEST(tr, test_single_write_single_read_within_one_sector);
    RUN_TEST(tr, test_several_writes_before_several_reads);
    RUN_TEST(tr, test_closing_file_during_read_without_invalidation);
}

int main()
{
    run_tests();
    return 0;
}