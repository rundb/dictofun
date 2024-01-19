// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2024, Roman Turkin
 */

#include "myfs.h"

#include <gtest/gtest.h>

#include <iostream>
using namespace std;

using namespace filesystem;

int sim_read(const struct myfs_config* c, myfs_block_t block, myfs_off_t off, void* buffer, myfs_size_t size);
int sim_prog(const struct myfs_config* c, myfs_block_t block, myfs_off_t off, const void* buffer, myfs_size_t size);
int sim_erase(const struct myfs_config* c, myfs_block_t block);
int sim_erase_multiple(const struct myfs_config* c, myfs_block_t block, uint32_t blocks_count);
int sim_sync(const struct myfs_config* c);

static constexpr uint32_t MEMORY_SIMULATION_SIZE{16*1024*1024};
static constexpr uint32_t MEMORY_SIMULATION_READ_SIZE{16};
static constexpr uint32_t MEMORY_SIMULATION_PROG_SIZE{256};
static constexpr uint32_t MEMORY_SIMULATION_BLOCK_SIZE{4096};
static constexpr uint8_t ERASED_MEMORY_CELL_VALUE{0xFFU};
static constexpr uint32_t MEMORY_SIMULATION_BLOCK_COUNT{MEMORY_SIMULATION_SIZE / MEMORY_SIMULATION_BLOCK_SIZE};
uint8_t memory_simulation[MEMORY_SIMULATION_SIZE];
uint8_t sim_read_buffer[MEMORY_SIMULATION_PROG_SIZE];
uint8_t sim_prog_buffer[MEMORY_SIMULATION_PROG_SIZE];

filesystem::myfs_config cut_config {
    .context = nullptr,
    .read = sim_read,
    .prog = sim_prog,
    .erase = sim_erase,
    .erase_multiple = sim_erase_multiple,
    .sync = sim_sync,

    .read_size = MEMORY_SIMULATION_READ_SIZE,
    .prog_size = MEMORY_SIMULATION_PROG_SIZE,
    .block_size = MEMORY_SIMULATION_BLOCK_SIZE,
    .block_count = MEMORY_SIMULATION_BLOCK_COUNT,

    .read_buffer = sim_read_buffer,
    .prog_buffer = sim_prog_buffer,
};

void dump_memory(uint8_t * buffer, uint32_t size);

class MyfsTest: public ::testing::Test {
protected:
    
    virtual void SetUp() 
    {
        memset(memory_simulation, ERASED_MEMORY_CELL_VALUE, MEMORY_SIMULATION_SIZE);
    }

    void mountCut() 
    {
        const auto mount_err = myfs_mount(cut);
        if (mount_err != 0)
        {
            const auto err = myfs_format(cut);
            if (err == 0)
            {
                const auto remount_err = myfs_mount(cut);
                ASSERT_EQ(remount_err, 0);
            }
            ASSERT_EQ(err, 0);
        }
    }

    // in the mounted system, create filesCount files with varying sizes and identifiers (content - file size in uint32_t, split to bytes)
    // file 1 - 1000 bytes
    // file 2 - 2000 bytes
    // file 3 - 4000 bytes, and so on
    // Identifier: '00000000', then '00000001', and so on.
    void createFiles(const uint32_t filesCount)
    {
        uint32_t file_size = 1000U;
        static constexpr uint32_t tmp_buf_size{16};
        uint8_t tmp[tmp_buf_size];
        for (uint32_t i = 0; i < filesCount; ++i) 
        {
            uint8_t file_id[myfs_file_descriptor::file_id_size + 1] {0};
            snprintf(reinterpret_cast<char*>(file_id), 9, "%08d", i);
            myfs_file_t file;
            const auto open_res = myfs_file_open(cut, file, file_id, MYFS_CREATE_FLAG);
            ASSERT_EQ(open_res, 0);
            
            uint32_t written_size = 0;
            for (auto i = 0; i < tmp_buf_size; i += sizeof(file_size)) 
            {
                memcpy(&tmp[i], &file_size, sizeof(file_size));
            }
            while (written_size < file_size)
            {
                const auto write_res = myfs_file_write(cut, file, tmp, tmp_buf_size);
                ASSERT_EQ(write_res, 0);
                written_size += tmp_buf_size;
            }
            
            const auto close_res = myfs_file_close(cut, file);
            ASSERT_EQ(close_res, 0);

            file_size *= 2;
            // dump_memory(&memory_simulation[0], 1024);
        }

    }

    myfs_t cut{cut_config};
};

TEST_F(MyfsTest, VerifySimulationCorrectness)
{
    uint8_t tmp[MEMORY_SIMULATION_PROG_SIZE] {0};
    uint8_t tmp_read[MEMORY_SIMULATION_PROG_SIZE] {0};
    {
        // check that per default memory is "erased"
        static constexpr uint32_t simple_read_size{16};
        const auto read_res = sim_read(&cut_config, 10, 0, tmp, simple_read_size);
        EXPECT_EQ(read_res, 0);
        for (auto i = 0; i < simple_read_size; ++i) 
        {
            EXPECT_EQ(tmp[i], ERASED_MEMORY_CELL_VALUE);
        }
    }
    {
        // "program", then verify, then erase, then verify
        static constexpr uint32_t verification_size{256};
        for (auto i = 0; i < sizeof(tmp); ++i) 
        {
            tmp[i] = static_cast<uint8_t>(i);
        }

        static constexpr uint32_t BLOCK_ID{150};
        static constexpr uint32_t TEST_SIZE{256};

        const auto prog_res = sim_prog(&cut_config, BLOCK_ID, TEST_SIZE, tmp, MEMORY_SIMULATION_PROG_SIZE);
        EXPECT_EQ(prog_res, 0);
        const auto read_res = sim_read(&cut_config, BLOCK_ID, TEST_SIZE, tmp_read, MEMORY_SIMULATION_PROG_SIZE);
        EXPECT_EQ(read_res, 0);

        for (auto i = 0; i < sizeof(tmp); ++i) 
        {
            EXPECT_EQ(tmp_read[i], static_cast<uint8_t>(i));
        }

        const auto erase_res = sim_erase(&cut_config, BLOCK_ID);
        EXPECT_EQ(erase_res, 0);

        const auto read_res_2 = sim_read(&cut_config, BLOCK_ID, TEST_SIZE, tmp_read, MEMORY_SIMULATION_PROG_SIZE);
        EXPECT_EQ(read_res_2, 0);
        for (auto i = 0; i < MEMORY_SIMULATION_PROG_SIZE; ++i) 
        {
            EXPECT_EQ(tmp_read[i], ERASED_MEMORY_CELL_VALUE);
        }
    }
    {
        // verify fault cases
        const auto read_res_1 = sim_read(&cut_config, 
            (MEMORY_SIMULATION_SIZE / MEMORY_SIMULATION_BLOCK_SIZE) - 1, 
            MEMORY_SIMULATION_BLOCK_SIZE - 1, tmp, 1);
        EXPECT_EQ(read_res_1, 0);

        const auto read_res_2 = sim_read(&cut_config, 
            (MEMORY_SIMULATION_SIZE / MEMORY_SIMULATION_BLOCK_SIZE) - 1, 
            MEMORY_SIMULATION_BLOCK_SIZE + 1, tmp, 1);

        EXPECT_NE(read_res_2, 0);
    }
}

TEST_F(MyfsTest, FormatAndMount) 
{
    {
        // attempt to mount not-formatted FS
        const auto err = myfs_mount(cut);
        EXPECT_NE(err, 0);
    }
    {
        const auto err = myfs_format(cut);
        EXPECT_EQ(err, 0);
        const auto mount_err = myfs_mount(cut);
        EXPECT_EQ(mount_err, 0);
    }
}

TEST_F(MyfsTest, FileSearchTestMediumQuantity) 
{
    mountCut();
    createFiles(14);

    const auto unmount_res = myfs_unmount(cut);
    EXPECT_EQ(unmount_res, 0);

    const auto mount_res = myfs_mount(cut);
    EXPECT_EQ(mount_res, 0);

    // dump_memory(&memory_simulation[0], 1024);
    uint8_t target_file_id = 9;
    uint8_t target_file_name[9]{0};
    ASSERT_TRUE(myfs_file_descriptor::file_id_size == sizeof(target_file_name) - 1);
    snprintf(reinterpret_cast<char *>(target_file_name), 9, "%08d", target_file_id);

    myfs_file_t file;
    const auto open_res = myfs_file_open(cut, file, target_file_name, MYFS_READ_FLAG);
    EXPECT_EQ(open_res, 0);

    uint8_t tmp[4]{0};
    uint32_t read_size{0};
    const auto read_res = myfs_file_read(cut, file, tmp, sizeof(tmp), read_size);
    EXPECT_EQ(read_res, 0);
    EXPECT_EQ(read_size, sizeof(tmp));
    uint32_t test_value;
    memcpy(reinterpret_cast<void*>(&test_value), tmp, sizeof(test_value));
    EXPECT_EQ(test_value, 512000);
    const auto close_res = myfs_file_close(cut, file);
    EXPECT_EQ(close_res, 0);
}

TEST_F(MyfsTest, FileSearchTestFirst2Files)
{
    mountCut();

    auto unmount_res = myfs_unmount(cut);
    EXPECT_EQ(unmount_res, 0);

    auto mount_res = myfs_mount(cut);
    EXPECT_EQ(mount_res, 0);

    // create a single file
    {
        uint32_t file_size = 1000U;
        static constexpr uint32_t tmp_buf_size{16};
        uint8_t tmp[tmp_buf_size];
        uint8_t file_id[myfs_file_descriptor::file_id_size + 1] {0};
        snprintf(reinterpret_cast<char*>(file_id), 9, "%08d", 1);
        myfs_file_t file;
        const auto open_res = myfs_file_open(cut, file, file_id, MYFS_CREATE_FLAG);
        ASSERT_EQ(open_res, 0);
            
        uint32_t written_size = 0;
        for (auto i = 0; i < tmp_buf_size; i += sizeof(file_size)) 
        {
            memcpy(&tmp[i], &file_size, sizeof(file_size));
        }
        while (written_size < file_size)
        {
            const auto write_res = myfs_file_write(cut, file, tmp, tmp_buf_size);
            ASSERT_EQ(write_res, 0);
            written_size += tmp_buf_size;
        }
            
        const auto close_res = myfs_file_close(cut, file);
        ASSERT_EQ(close_res, 0);
    }

    unmount_res = myfs_unmount(cut);
    EXPECT_EQ(unmount_res, 0);

    mount_res = myfs_mount(cut);
    EXPECT_EQ(mount_res, 0);

    // open the just created file
    {
        uint8_t target_file_id = 1;
        uint8_t target_file_name[9]{0};
        snprintf(reinterpret_cast<char *>(target_file_name), 9, "%08d", target_file_id);

        myfs_file_t file;
        const auto open_res = myfs_file_open(cut, file, target_file_name, MYFS_READ_FLAG);
        EXPECT_EQ(open_res, 0);

        uint8_t tmp[4]{0};
        uint32_t read_size{0};
        const auto read_res = myfs_file_read(cut, file, tmp, sizeof(tmp), read_size);
        EXPECT_EQ(read_res, 0);
        EXPECT_EQ(read_size, sizeof(tmp));
        uint32_t test_value;
        memcpy(reinterpret_cast<void*>(&test_value), tmp, sizeof(test_value));
        EXPECT_EQ(test_value, 1000);
        const auto close_res = myfs_file_close(cut, file);
        EXPECT_EQ(close_res, 0);
    }

    // run second iteration of opening and closing
    unmount_res = myfs_unmount(cut);
    EXPECT_EQ(unmount_res, 0);

    mount_res = myfs_mount(cut);
    EXPECT_EQ(mount_res, 0);

    {
        uint32_t file_size = 2000U;
        static constexpr uint32_t tmp_buf_size{16};
        uint8_t tmp[tmp_buf_size];
        uint8_t file_id[myfs_file_descriptor::file_id_size + 1] {0};
        snprintf(reinterpret_cast<char*>(file_id), 9, "%08d", 2);
        myfs_file_t file;
        const auto open_res = myfs_file_open(cut, file, file_id, MYFS_CREATE_FLAG);
        ASSERT_EQ(open_res, 0);
            
        uint32_t written_size = 0;
        for (auto i = 0; i < tmp_buf_size; i += sizeof(file_size)) 
        {
            memcpy(&tmp[i], &file_size, sizeof(file_size));
        }
        while (written_size < file_size)
        {
            const auto write_res = myfs_file_write(cut, file, tmp, tmp_buf_size);
            ASSERT_EQ(write_res, 0);
            written_size += tmp_buf_size;
        }
            
        const auto close_res = myfs_file_close(cut, file);
        ASSERT_EQ(close_res, 0);
    }
    unmount_res = myfs_unmount(cut);
    EXPECT_EQ(unmount_res, 0);

    mount_res = myfs_mount(cut);
    EXPECT_EQ(mount_res, 0);

    // open the just created file
    {
        uint8_t target_file_id = 2;
        uint8_t target_file_name[9]{0};
        snprintf(reinterpret_cast<char *>(target_file_name), 9, "%08d", target_file_id);

        myfs_file_t file;
        const auto open_res = myfs_file_open(cut, file, target_file_name, MYFS_READ_FLAG);
        ASSERT_EQ(open_res, 0);

        uint8_t tmp[4]{0};
        uint32_t read_size{0};
        const auto read_res = myfs_file_read(cut, file, tmp, sizeof(tmp), read_size);
        EXPECT_EQ(read_res, 0);
        EXPECT_EQ(read_size, sizeof(tmp));
        uint32_t test_value;
        memcpy(reinterpret_cast<void*>(&test_value), tmp, sizeof(test_value));
        EXPECT_EQ(test_value, 2000);
        const auto close_res = myfs_file_close(cut, file);
        EXPECT_EQ(close_res, 0);
    }
    // open the first file
    {
        uint8_t target_file_id = 1;
        uint8_t target_file_name[9]{0};
        snprintf(reinterpret_cast<char *>(target_file_name), 9, "%08d", target_file_id);

        myfs_file_t file;
        const auto open_res = myfs_file_open(cut, file, target_file_name, MYFS_READ_FLAG);
        ASSERT_EQ(open_res, 0);

        uint8_t tmp[4]{0};
        uint32_t read_size{0};
        const auto read_res = myfs_file_read(cut, file, tmp, sizeof(tmp), read_size);
        EXPECT_EQ(read_res, 0);
        EXPECT_EQ(read_size, sizeof(tmp));
        uint32_t test_value;
        memcpy(reinterpret_cast<void*>(&test_value), tmp, sizeof(test_value));
        EXPECT_EQ(test_value, 1000);
        const auto close_res = myfs_file_close(cut, file);
        EXPECT_EQ(close_res, 0);
    }
    unmount_res = myfs_unmount(cut);
    EXPECT_EQ(unmount_res, 0);
}

TEST_F(MyfsTest, Create3rdFile) 
{
    mountCut();
    createFiles(3);

    const auto unmount_res = myfs_unmount(cut);
    EXPECT_EQ(unmount_res, 0);

    const auto mount_res = myfs_mount(cut);
    EXPECT_EQ(mount_res, 0);

    // create 3rd file
    static constexpr uint32_t third_file_size = 5021U;    
    {
        static constexpr uint32_t tmp_buf_size{16};
        uint8_t tmp[tmp_buf_size];
        uint8_t file_id[myfs_file_descriptor::file_id_size + 1] {0};
        snprintf(reinterpret_cast<char*>(file_id), 9, "%08d", 3);
        myfs_file_t file;
        const auto open_res = myfs_file_open(cut, file, file_id, MYFS_CREATE_FLAG);
        ASSERT_EQ(open_res, 0);
            
        uint32_t written_size = 0;
        uint32_t file_size = third_file_size;
        for (auto i = 0; i < tmp_buf_size; i += sizeof(third_file_size)) 
        {
            memcpy(&tmp[i], &file_size, sizeof(file_size));
        }
        while (written_size < third_file_size)
        {
            const auto write_size = std::min(third_file_size - written_size, tmp_buf_size);
            const auto write_res = myfs_file_write(cut, file, tmp, write_size);
            ASSERT_EQ(write_res, 0);
            written_size += write_size;
        }
            
        const auto close_res = myfs_file_close(cut, file);
        ASSERT_EQ(close_res, 0);
        // dump_memory(&memory_simulation[0], 1024);
        cout << endl;
    }

    // now open and close first 3 files
    uint8_t target_file_name[9]{0};
    myfs_file_t file;

    dump_memory(&memory_simulation[0], 1024);

    snprintf(reinterpret_cast<char *>(target_file_name), 9, "%08d", 0);
    const auto open_res_1 = myfs_file_open(cut, file, target_file_name, MYFS_READ_FLAG);
    ASSERT_EQ(open_res_1, 0);
    const auto close_res_1 = myfs_file_close(cut, file);
    EXPECT_EQ(close_res_1, 0);

    snprintf(reinterpret_cast<char *>(target_file_name), 9, "%08d", 1);
    const auto open_res_2 = myfs_file_open(cut, file, target_file_name, MYFS_READ_FLAG);
    ASSERT_EQ(open_res_2, 0);
    const auto close_res_2 = myfs_file_close(cut, file);
    EXPECT_EQ(close_res_2, 0);

    // now check that open() doesn't open non-existent files
    snprintf(reinterpret_cast<char *>(target_file_name), 9, "%08d", 9);
    const auto open_res_9 = myfs_file_open(cut, file, target_file_name, MYFS_READ_FLAG);
    ASSERT_NE(open_res_9, 0);

    // Open 3rd file, also make sure that it's size is equal to what was created above
    snprintf(reinterpret_cast<char *>(target_file_name), 9, "%08d", 3);
    const auto open_res_3 = myfs_file_open(cut, file, target_file_name, MYFS_READ_FLAG);
    EXPECT_EQ(open_res_3, 0);
    const auto close_res_3 = myfs_file_close(cut, file);
    EXPECT_EQ(close_res_3, 0);
}

int sim_read(const struct myfs_config* c, myfs_block_t block, myfs_off_t off, void* buffer, myfs_size_t size) 
{
    if (nullptr == c || nullptr == buffer) {
        return -1;   
    }
    const auto start_address = block * c->block_size + off;
    const auto end_address = block * c->block_size + off + size;

    if (end_address > MEMORY_SIMULATION_SIZE)
    {
        return -2;
    }

    memcpy(buffer, &memory_simulation[start_address], size);

    return 0;
}

int sim_prog(const struct myfs_config* c, myfs_block_t block, myfs_off_t off, const void* buffer, myfs_size_t size) 
{
    if (nullptr == c || nullptr == buffer) {
        return -1;   
    }
    const auto start_address = block * c->block_size + off;
    const auto end_address = block * c->block_size + off + size;

    if (end_address > MEMORY_SIMULATION_SIZE)
    {
        return -2;
    }

    memcpy(&memory_simulation[start_address], buffer, size);
    return 0;
}

int sim_erase(const struct myfs_config* c, myfs_block_t block) {
    if (nullptr == c) 
    {
        return -1;
    }
    const auto sector_start = block * c->block_size;
    const auto sector_end = (block + 1) * c->block_size - 1;

    if (sector_end >= MEMORY_SIMULATION_SIZE) 
    {
        return -2;
    }

    memset(&memory_simulation[sector_start], ERASED_MEMORY_CELL_VALUE, c->block_size);
    return 0;
}


int sim_erase_multiple(const struct myfs_config* c, myfs_block_t block, uint32_t blocks_count) 
{
    if (nullptr == c) 
    {
        return -1;
    }

    for (auto i = 0; i < blocks_count; ++i) {
        const auto result = sim_erase(c, block + i);
        if (result != 0) 
        {
            return result;
        }
    }
    return 0;
}

int sim_sync(const struct myfs_config* c) {
    return 0;
}

void dump_memory(uint8_t * buffer, uint32_t size)
{
    const int elements_per_row = 16;

    for (uint32_t i = 0; i < size; i += elements_per_row) {
        // Print address
        std::cout << std::hex << std::setw(8) << std::setfill('0') << i << ": ";

        // Print data in the row
        for (int j = 0; j < elements_per_row; ++j) {
            if (i + j < size) {
                std::cout << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(buffer[i + j]) << " ";
            } else {
                std::cout << "   "; // Padding for the last row
            }
        }

        // Print ASCII representation
        std::cout << "  ";
        for (int j = 0; j < elements_per_row; ++j) {
            if (i + j < size) {
                char c = (buffer[i + j] >= 32 && buffer[i + j] <= 126) ? static_cast<char>(buffer[i + j]) : '.';
                std::cout << c;
            } else {
                std::cout << " ";
            }
        }

        std::cout << std::endl;
    }
}
