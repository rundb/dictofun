// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "task_memory.h"

#include "FreeRTOS.h"
#include "task.h"

#include "lfs.h"
#include "nrf_log.h"
#include "littlefs_access.h"


namespace memory 
{

static constexpr size_t memtest_buffer_size{256};
static uint8_t memtest_buffer[memtest_buffer_size]{0};
static lfs_file_t memtest_file;
static lfs_file_config memtest_file_config;

void launch_test_1(flash::SpiFlash& flash)
{
    NRF_LOG_INFO("task memory: launching chip erase. Memory task shall not accept commands during the execution of this command.");
    const auto start_tick = xTaskGetTickCount();
    flash.eraseChip();
    static constexpr uint32_t max_chip_erase_duration{50000};
    while (flash.isBusy() && (xTaskGetTickCount() - start_tick) < max_chip_erase_duration)
    {
        vTaskDelay(100);
    }
    const auto end_tick = xTaskGetTickCount();
    if ((end_tick - start_tick) > max_chip_erase_duration)
    {
        NRF_LOG_WARNING("task memory: chip erase timed out");
    }
    else
    {
        NRF_LOG_INFO("task memory: chip erase took %d ms", end_tick - start_tick);
    }
}

void launch_test_2(flash::SpiFlash& flash)
{
    NRF_LOG_INFO("task memory: launching read-program-read-erase-program test. \n"\
                    "Memory task shall not accept commands during the execution of this command.");
    constexpr uint32_t test_area_start_address{16*1024*1024-2*4096}; // second sector from the end.  
    constexpr uint32_t test_data_size{256};
    constexpr uint32_t test_erase_size{4096};
    uint8_t test_data[test_data_size]{0};

    // ===== Step 1: read the current content of the memory
    const auto read_1_start_tick{xTaskGetTickCount()};
    const auto read_1_res = flash.read(test_area_start_address, test_data, test_data_size);
    const auto read_1_end_tick{xTaskGetTickCount()};
    if (memory::SpiNorFlashIf::Result::OK != read_1_res)
    {
        NRF_LOG_ERROR("mem: failed to read flash memory");
        return;
    }
    bool is_area_erased{true};
    for (auto i = 0UL; i < test_data_size; ++i)
    {
        if (test_data[i] != 0xFF) is_area_erased = false;
    }
    if (!is_area_erased)
    {
        NRF_LOG_ERROR("mem: test area is not erased. aborting");
        return;
    }
    NRF_LOG_INFO("read took %d ms", read_1_end_tick - read_1_start_tick);

    // ==== Step 2: program a single page with predefined values
    for (auto i = 0UL; i < test_data_size; ++i)
    {
        test_data[i] = i;
    }
    const auto prog_start_tick{xTaskGetTickCount()};
    const auto program_res = flash.program(test_area_start_address, test_data, test_data_size);
    const auto prog_end_tick{xTaskGetTickCount()};
    if (memory::SpiNorFlashIf::Result::OK != program_res)
    {
        NRF_LOG_ERROR("mem: failed to read flash memory");
        return;
    }
    NRF_LOG_INFO("prog took %d ms", prog_end_tick - prog_start_tick);

    // ==== Step 3: read back the programmed content
    memset(test_data, 0, test_data_size);
    const auto read_2_start_tick{xTaskGetTickCount()};
    const auto read_2_res = flash.read(test_area_start_address, test_data, test_data_size);
    const auto read_2_end_tick{xTaskGetTickCount()};
    if (memory::SpiNorFlashIf::Result::OK != read_2_res)
    {
        NRF_LOG_ERROR("mem: failed to read flash memory");
        return;
    }
    bool is_area_programmed{true};
    for (auto i = 0UL; i < test_data_size; ++i)
    {
        if (test_data[i] != i) is_area_programmed = false;
    }
    if (!is_area_programmed)
    {
        NRF_LOG_ERROR("mem: test area has not been programmed. aborting");
        return;
    }
    NRF_LOG_INFO("read 2 took %d ms", read_2_end_tick - read_2_start_tick);

    // ===== Step 2.1: modify a single byte that was previously not programmed
    test_data[0] = 0x6;
    const auto reprogram_result = flash.program(test_area_start_address + 7, test_data, 1);
    if (memory::SpiNorFlashIf::Result::OK != reprogram_result)
    {
        NRF_LOG_ERROR("reprogram failed");
    }
    test_data[0] = 0;
    const auto reread_result = flash.read(test_area_start_address + 7, test_data, 1);
    if (memory::SpiNorFlashIf::Result::OK != reread_result)
    {
        NRF_LOG_ERROR("reread failed");
    }
    if (test_data[0] != 0x6)
    {
        NRF_LOG_ERROR("reprogram failed: %x != 6", test_data[0]);
    }

    // ==== Step 4: erase the sector we just programmed
    const auto erase_start_tick{xTaskGetTickCount()};
    const auto erase_res = flash.erase(test_area_start_address, test_erase_size);
    const auto erase_end_tick{xTaskGetTickCount()};

    if (memory::SpiNorFlashIf::Result::OK != erase_res)
    {
        NRF_LOG_ERROR("mem: failed to erase sector");
        return;
    }
    NRF_LOG_INFO("erase took %d ms", erase_end_tick - erase_start_tick);

    // ==== Step 5: verify that the test area has been erased.
    memset(test_data, 0, test_data_size);
    const auto read_3_start_tick{xTaskGetTickCount()};
    const auto read_3_res = flash.read(test_area_start_address, test_data, test_data_size);
    const auto read_3_end_tick{xTaskGetTickCount()};
    if (memory::SpiNorFlashIf::Result::OK != read_3_res)
    {
        NRF_LOG_ERROR("mem: failed to read flash memory");
        return;
    }
    is_area_erased = true;
    for (auto i = 0UL; i < test_data_size; ++i)
    {
        if (test_data[i] != 0xFF) 
        {
            is_area_erased = false;
            NRF_LOG_ERROR("mem: %x@%x", test_data[i], i);
            break;
        }
    }
    if (!is_area_erased)
    {
        NRF_LOG_ERROR("mem: test area is not erased. aborting");
        return;
    }
    NRF_LOG_INFO("read3 took %d ms", read_3_end_tick - read_3_start_tick);
}

void launch_test_3(lfs_t& lfs, const lfs_config& lfs_configuration)
{
    const auto test_start_tick{xTaskGetTickCount()};
    NRF_LOG_INFO("mem: launching simple LFS operation test");
    // ============ Step 1: initialize the FS (and format it, if necessary)
    auto err = lfs_mount(&lfs, &lfs_configuration);
    if (err != 0)
    {
        NRF_LOG_INFO("mem: formatting FS");
        err = lfs_format(&lfs, &lfs_configuration);
        if (err != 0)
        {
            NRF_LOG_ERROR("mem: failed to format LFS (%d)", err);
            return;
        }
        err = lfs_mount(&lfs, &lfs_configuration);
        if (err != 0)
        {
            NRF_LOG_ERROR("mem: failed to mount LFS after formatting");
            return;
        }
    }
    // ============ Step 2: create a file and write content into it
    NRF_LOG_INFO("mem: creating first file");
    memset(&memtest_file_config, 0, sizeof(memtest_file_config));
    memtest_file_config.buffer = memtest_buffer;
    memtest_file_config.attr_count = 0;
    const auto create_result = lfs_file_opencfg(&lfs, &memtest_file, "record00", LFS_O_CREAT | LFS_O_WRONLY, &memtest_file_config);
    if (create_result!= 0)
    {
        NRF_LOG_ERROR("lfs: failed to create a file (err=%d)", create_result);
    }

    constexpr size_t test_data_size{512};
    uint8_t test_data[test_data_size];
    for (size_t i = 0; i < test_data_size; ++i)
    {
        test_data[i] = i & 0xFF;
    }
    NRF_LOG_INFO("mem: writing data to opened file");
    // Enforce overflow of a single sector size to provoke an erase operation
    size_t written_data_size{0};
    const auto write_start_tick{xTaskGetTickCount()};
    for (int i = 0; i < 4; ++i)
    {
        const auto write_result = lfs_file_write(&lfs, &memtest_file, test_data, sizeof(test_data));
        written_data_size += sizeof(test_data);
        if (write_result!= sizeof(test_data))
        {
            NRF_LOG_ERROR("lfs: failed to write into a file (%d)", write_result);
            return;
        }
    }
    const auto write_end_tick{xTaskGetTickCount()};
        
    // ============ Step 3: close the file
    NRF_LOG_INFO("mem: closing file");
    const auto close_result_0 = lfs_file_close(&lfs, &memtest_file);
    if (close_result_0!= 0)
    {
        NRF_LOG_ERROR("lfs: failed to close file after writing");
        return;
    }
    memset(test_data, 0, test_data_size);


    NRF_LOG_INFO("mem: creating second file");
    memset(&memtest_file_config, 0, sizeof(memtest_file_config));
    memtest_file_config.buffer = memtest_buffer;
    memtest_file_config.attr_count = 0;
    const auto create_2_result = lfs_file_opencfg(&lfs, &memtest_file, "record01", LFS_O_CREAT | LFS_O_WRONLY, &memtest_file_config);
    if (create_2_result!= 0)
    {
        NRF_LOG_ERROR("lfs: failed to create a file (err=%d)", create_2_result);
    }

    NRF_LOG_INFO("mem: writing data to opened file");
    // Enforce overflow of a single sector size to provoke an erase operation

    for (int i = 0; i < 10; ++i)
    {
        const auto write_result = lfs_file_write(&lfs, &memtest_file, test_data, sizeof(test_data));
        written_data_size += sizeof(test_data);
        if (write_result!= sizeof(test_data))
        {
            NRF_LOG_ERROR("lfs: failed to write into a file (%d)", write_result);
            return;
        }
    }

    const auto close_result_1 = lfs_file_close(&lfs, &memtest_file);
    if (close_result_1!= 0)
    {
        NRF_LOG_ERROR("lfs: failed to close file after writing");
        return;
    }

    // ============ Step 4: open the file for read
    NRF_LOG_INFO("mem: opening file for read");
    lfs_file_t file;
    const auto open_to_read_result = lfs_file_open(&lfs, &file, "record00", LFS_O_RDONLY);
    if (open_to_read_result!= 0)
    {
        NRF_LOG_ERROR("lfs: failed to open file for reading");
        return;
    }
    const auto read_result = lfs_file_read(&lfs, &file, test_data, sizeof(test_data));
    if (read_result != sizeof(test_data))
    {
        NRF_LOG_ERROR("lfs: failed to read file content");
        return;
    }

    // ============ Step 5: close the file
    NRF_LOG_INFO("mem: closing file");
    const auto close_result_2 = lfs_file_close(&lfs, &file);
    if (close_result_2 != 0)
    {
        NRF_LOG_ERROR("lfs: failed to close file after opening");
        return;
    }

    // ============ Step 6: validate a portion of the file
    if (test_data[1] != 1 || test_data[9] != 9 || test_data[15] != 15)
    {
        NRF_LOG_ERROR("lfs: failed to validate file content");
        return;
    }
    lfs_unmount(&lfs);
    const auto test_end_tick{xTaskGetTickCount()};
    NRF_LOG_INFO("LFS test took %d ms", (test_end_tick - test_start_tick));
    NRF_LOG_INFO("LFS write throughput: %d ms, %d bytes, %d bytes/ms", 
        (write_end_tick - write_start_tick + 1),  
        written_data_size,
        written_data_size / (write_end_tick - write_start_tick + 1)
        );
}

void launch_test_4(lfs_t& lfs)
{
    NRF_LOG_INFO("mem: launching LFS write access test");
    // Step 1. Fetch last record's name
    char tmp_record_name[10]{0};
    uint32_t tmp_length{0};
    const auto name_read_result = memory::filesystem::get_latest_file_name(lfs, tmp_record_name, tmp_length);
    if (result::Result::OK != name_read_result)
    {
        NRF_LOG_ERROR("failed to read latest record name. aborting the test");
        return;
    }
    memory::Context context;
    memory::generate_next_file_name(tmp_record_name, context);
    // ============ Step 2: create a file and write content into it
    NRF_LOG_DEBUG("mem: creating file");
    const auto create_result = memory::filesystem::create_file(lfs, tmp_record_name);
    if (result::Result::OK != create_result)
    {
        NRF_LOG_ERROR("lfs: failed to create a file");
    }

    constexpr size_t test_data_size{512};
    uint8_t test_data[test_data_size];
    for (size_t i = 0; i < test_data_size; ++i)
    {
        test_data[i] = i & 0xFF;
    }
    const auto write_result = memory::filesystem::write_data(lfs, test_data, test_data_size);
    if (result::Result::OK != write_result)
    {
        NRF_LOG_ERROR("memtest: write data failed");
    }
    
    // ============ Step 3: close the file
    NRF_LOG_INFO("mem: closing file");
    const auto close_result = memory::filesystem::close_file(lfs, tmp_record_name);
    if (close_result != result::Result::OK)
    {
        NRF_LOG_ERROR("lfs: failed to close file after writing");
        return;
    }
    
}

}