// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "task_memory.h"

#include "FreeRTOS.h"
#include "task.h"

#include "myfs_access.h"
#include "nrf_log.h"

namespace memory
{

void launch_test_1(flash::SpiFlash& flash)
{
    NRF_LOG_INFO("task memory: launching chip erase. Memory task shall not accept commands during "
                 "the execution of this command.");
    const auto start_tick = xTaskGetTickCount();
    flash.eraseChip();
    static constexpr uint32_t max_chip_erase_duration{50000};
    while(flash.isBusy() && (xTaskGetTickCount() - start_tick) < max_chip_erase_duration)
    {
        vTaskDelay(100);
    }
    const auto end_tick = xTaskGetTickCount();
    if((end_tick - start_tick) > max_chip_erase_duration)
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
    NRF_LOG_INFO("task memory: launching read-program-read-erase-program test. \n"
                 "Memory task shall not accept commands during the execution of this command.");
    constexpr uint32_t test_area_start_address{16 * 1024 * 1024 -
                                               2 * 4096}; // second sector from the end.
    constexpr uint32_t test_data_size{256};
    constexpr uint32_t test_erase_size{4096};
    uint8_t test_data[test_data_size]{0};

    // ===== Step 1: read the current content of the memory
    const auto read_1_start_tick{xTaskGetTickCount()};
    const auto read_1_res = flash.read(test_area_start_address, test_data, test_data_size);
    const auto read_1_end_tick{xTaskGetTickCount()};
    if(memory::SpiNorFlashIf::Result::OK != read_1_res)
    {
        NRF_LOG_ERROR("mem: failed to read flash memory");
        return;
    }
    bool is_area_erased{true};
    for(auto i = 0UL; i < test_data_size; ++i)
    {
        if(test_data[i] != 0xFF)
            is_area_erased = false;
    }
    if(!is_area_erased)
    {
        NRF_LOG_ERROR("mem: test area is not erased. aborting");
        return;
    }
    NRF_LOG_INFO("read took %d ms", read_1_end_tick - read_1_start_tick);

    // ==== Step 2: program a single page with predefined values
    for(auto i = 0UL; i < test_data_size; ++i)
    {
        test_data[i] = i;
    }
    const auto prog_start_tick{xTaskGetTickCount()};
    const auto program_res = flash.program(test_area_start_address, test_data, test_data_size);
    const auto prog_end_tick{xTaskGetTickCount()};
    if(memory::SpiNorFlashIf::Result::OK != program_res)
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
    if(memory::SpiNorFlashIf::Result::OK != read_2_res)
    {
        NRF_LOG_ERROR("mem: failed to read flash memory");
        return;
    }
    bool is_area_programmed{true};
    for(auto i = 0UL; i < test_data_size; ++i)
    {
        if(test_data[i] != i)
            is_area_programmed = false;
    }
    if(!is_area_programmed)
    {
        NRF_LOG_ERROR("mem: test area has not been programmed. aborting");
        return;
    }
    NRF_LOG_INFO("read 2 took %d ms", read_2_end_tick - read_2_start_tick);

    // ===== Step 2.1: modify a single byte that was previously not programmed
    test_data[0] = 0x6;
    const auto reprogram_result = flash.program(test_area_start_address + 7, test_data, 1);
    if(memory::SpiNorFlashIf::Result::OK != reprogram_result)
    {
        NRF_LOG_ERROR("reprogram failed");
    }
    test_data[0] = 0;
    const auto reread_result = flash.read(test_area_start_address + 7, test_data, 1);
    if(memory::SpiNorFlashIf::Result::OK != reread_result)
    {
        NRF_LOG_ERROR("reread failed");
    }
    if(test_data[0] != 0x6)
    {
        NRF_LOG_ERROR("reprogram failed: %x != 6", test_data[0]);
    }

    // ==== Step 4: erase the sector we just programmed
    const auto erase_start_tick{xTaskGetTickCount()};
    const auto erase_res = flash.erase(test_area_start_address, test_erase_size);
    const auto erase_end_tick{xTaskGetTickCount()};

    if(memory::SpiNorFlashIf::Result::OK != erase_res)
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
    if(memory::SpiNorFlashIf::Result::OK != read_3_res)
    {
        NRF_LOG_ERROR("mem: failed to read flash memory");
        return;
    }
    is_area_erased = true;
    for(auto i = 0UL; i < test_data_size; ++i)
    {
        if(test_data[i] != 0xFF)
        {
            is_area_erased = false;
            NRF_LOG_ERROR("mem: %x@%x", test_data[i], i);
            break;
        }
    }
    if(!is_area_erased)
    {
        NRF_LOG_ERROR("mem: test area is not erased. aborting");
        return;
    }
    NRF_LOG_INFO("read3 took %d ms", read_3_end_tick - read_3_start_tick);
}

void launch_test_3(myfs_t& myfs, const myfs_config& myfs_configuration)
{
    NRF_LOG_INFO("mem: launching simple MyFS operation test");
    // ============ Step 1: initialize the FS (and format it, if necessary)
    vTaskDelay(20);
    auto err = myfs_mount(myfs);
    if(err != 0)
    {
        vTaskDelay(20);
        NRF_LOG_INFO("mem: formatting MyFS");
        err = myfs_format(myfs);
        if(err != 0)
        {
            NRF_LOG_ERROR("mem: failed to format MyFS (%d)", err);
            return;
        }
        err = myfs_mount(myfs);
        if(err != 0)
        {
            NRF_LOG_ERROR("mem: failed to mount MyFS after formatting");
            return;
        }
    }

    // ============= Step 2: create a test record, open file and write it
    NRF_LOG_INFO("mem: creating 2 files");
    ::filesystem::myfs_file_t test_file;
    uint8_t test_file_id[8]{0, 0, 23, 11, 1, 21, 30, 12};
    const auto create_result = myfs_file_open(myfs, test_file, test_file_id, ::filesystem::MYFS_CREATE_FLAG);

    if (create_result != 0)
    {
        NRF_LOG_ERROR("mem: failed to open file for write (err %d)", create_result);
        return;
    }

    uint8_t tmp_data[32];
    for (uint8_t i = 0; i < sizeof(tmp_data); ++i)
    {
        tmp_data[i] = i;
    }

    for (auto i = 0; i < 420; i += sizeof(tmp_data))
    {
        const auto write_res = myfs_file_write(myfs, test_file, tmp_data, sizeof(tmp_data));
        if (write_res < 0)
        {
            NRF_LOG_ERROR("memtest: write has failed (pos %d)", i);
        }
    }

    const auto close_result = myfs_file_close(myfs, test_file);
    if (close_result != 0)
    {
        NRF_LOG_ERROR("mem: failed to close file after write (err %d)", close_result);
        return;
    }

    test_file_id[0]++;

    const auto create_result_2 = myfs_file_open(myfs, test_file, test_file_id, ::filesystem::MYFS_CREATE_FLAG);

    if (create_result_2 != 0)
    {
        NRF_LOG_ERROR("memtest: failed to create 2nd file (err %d)", create_result_2);
        return;
    }

    const auto write_res = myfs_file_write(myfs, test_file, tmp_data, sizeof(tmp_data));
    if (write_res < 0)
    {
        NRF_LOG_ERROR("memtest: write 2 has failed (err %d)", write_res);
    }

    const auto close_res_2 = myfs_file_close(myfs, test_file);
    if (close_res_2 != 0)
    {
        NRF_LOG_ERROR("mem: failed to close file 2 after write (err %d)", close_res_2);
        return;
    }

    test_file_id[0]--;
    const auto open_result_1 = myfs_file_open(myfs, test_file, test_file_id, ::filesystem::MYFS_READ_FLAG);
    if (open_result_1 != 0)
    {
        NRF_LOG_ERROR("mem: failed to open file 1 for read (err %d)", open_result_1);
        return;
    }
    memset(tmp_data, 0, sizeof(tmp_data));

    uint32_t actually_read_size;
    do 
    {
        const auto read_result = myfs_file_read(myfs, test_file, tmp_data, sizeof(tmp_data), actually_read_size);
        if (0 != read_result)
        {
            NRF_LOG_ERROR("memtest: data read failed");
            return;
        }
        NRF_LOG_INFO("memtest: read %d bytes", actually_read_size);
    } while (actually_read_size != 0);

    const auto close_res_3 = myfs_file_close(myfs, test_file);
    if (close_res_3 != 0)
    {
        NRF_LOG_ERROR("mem: failed to close file 3 after read (err %d)", close_res_3);
        return;
    }
}

void launch_test_5(flash::SpiFlash& flash,const uint32_t range_start, const uint32_t range_end)
{
    NRF_LOG_INFO("memory dump (0x%x:0x%x)", range_start, range_end);
    
    if (range_start > range_end)
    {
        NRF_LOG_WARNING("memory dump: incorrect address range");
        return;
    }
    static constexpr uint32_t single_read_size{16};
    uint8_t buf[single_read_size]{0};
    char str[9 + single_read_size * 3 + 2] {0};
    for (uint32_t i = range_start; i < range_end; i += single_read_size)
    {
        flash.read(i, buf, single_read_size);
        snprintf(str, sizeof(str), "%08x: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
            static_cast<unsigned int>(i),
            buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],
            buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]
        );
        NRF_LOG_INFO("%s", str);
        vTaskDelay(50);
    }
}

} // namespace memory