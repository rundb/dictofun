// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "task_memory.h"
#include "nrf_log.h"

#include "FreeRTOS.h"
#include "task.h"

#include "spi_flash.h"
#include "boards.h"
#include "block_api.h"
#include "littlefs_access.h"

#include "task_ble.h"

#include "lfs.h"

namespace memory
{

spi::Spi flash_spi(0, SPI_FLASH_CS_PIN);

static const spi::Spi::Configuration flash_spi_config{NRF_DRV_SPI_FREQ_2M,
                                                      NRF_DRV_SPI_MODE_0,
                                                      NRF_DRV_SPI_BIT_ORDER_MSB_FIRST,
                                                      SPI_FLASH_SCK_PIN,
                                                      SPI_FLASH_MOSI_PIN,
                                                      SPI_FLASH_MISO_PIN};

flash::SpiFlash flash{flash_spi, vTaskDelay};
constexpr uint32_t flash_page_size{256};
constexpr uint32_t flash_sector_size{4096};
constexpr uint32_t flash_total_size{16*1024*1024-4096};
constexpr uint32_t flash_sectors_count{flash_total_size / flash_sector_size};

constexpr size_t CACHE_SIZE{2 * flash_page_size};
constexpr size_t LOOKAHEAD_BUFFER_SIZE{16};
uint8_t prog_buffer[CACHE_SIZE];
uint8_t read_buffer[CACHE_SIZE];
uint8_t lookahead_buffer[CACHE_SIZE];

lfs_t lfs;

const struct lfs_config lfs_configuration = {
    .read  = memory::block_device::read,
    .prog  = memory::block_device::program,
    .erase = memory::block_device::erase,
    .sync  = memory::block_device::sync,

    // block device configuration
    .read_size = 16,
    .prog_size = flash_page_size,
    .block_size = flash_sector_size,
    .block_count = flash_sectors_count,
    .block_cycles = 500,
    .cache_size = flash_page_size,
    .lookahead_size = LOOKAHEAD_BUFFER_SIZE,
    .read_buffer = read_buffer,
    .prog_buffer = prog_buffer,
    .lookahead_buffer = lookahead_buffer,
};


constexpr uint32_t cmd_wait_ticks{5};
constexpr uint32_t ble_command_wait_ticks{5};

// This element allocated statically, as it's rather big (~260 bytes)
ble::FileDataFromMemoryQueueElement data_queue_elem;

void launch_test_1();
void launch_test_2();
void launch_test_3();

static bool is_ble_access_allowed();
void process_request_from_ble(Context& context, ble::CommandToMemory command_id, ble::fts::file_id_type file_id);

void task_memory(void * context_ptr)
{
    NRF_LOG_INFO("task memory: initialized");
    Context& context = *(reinterpret_cast<Context *>(context_ptr));
    CommandQueueElement command;
    ble::CommandToMemoryQueueElement command_from_ble;

    flash_spi.init(flash_spi_config);
    flash.init();
    flash.reset();
    uint8_t jedec_id[6];
    flash.readJedecId(jedec_id);
    NRF_LOG_INFO("memory id: %x-%x-%x", jedec_id[0], jedec_id[1], jedec_id[2]);
    
    memory::block_device::register_flash_device(&flash, flash_sector_size, flash_page_size, flash_total_size);

    while(1)
    {
        const auto cmd_queue_receive_status = xQueueReceive(
            context.command_queue,
            reinterpret_cast<void *>(&command),
            cmd_wait_ticks);
        if (pdPASS == cmd_queue_receive_status)
        {
            if (command.command_id == Command::LAUNCH_TEST_1)
            {
                launch_test_1();
            }
            else if (command.command_id == Command::LAUNCH_TEST_2)
            {
                launch_test_2();
            }
            else if (command.command_id == Command::LAUNCH_TEST_3)
            {
                launch_test_3();
            }
        }
        const auto cmd_from_ble_queue_receive_status = xQueueReceive(
            context.command_from_ble_queue,
            reinterpret_cast<void *>(&command_from_ble),
            ble_command_wait_ticks);
        if (pdPASS == cmd_from_ble_queue_receive_status)
        {
            process_request_from_ble(context, command_from_ble.command_id, command_from_ble.file_id);
        }
    }
}

// TODO: implement access 
static bool is_ble_access_allowed()
{
    return true;
}

void convert_file_id_to_string(ble::fts::file_id_type file_id, char * buffer)
{
    memcpy(buffer, &file_id, sizeof(ble::fts::file_id_type));
}

void process_request_from_ble(Context& context, ble::CommandToMemory command_id, ble::fts::file_id_type file_id)
{
    ble::StatusFromMemoryQueueElement status{ble::StatusFromMemory::OK, 0};
    if (!is_ble_access_allowed())
    {
        status.status = ble::StatusFromMemory::ERROR_PERMISSION_DENIED;
        status.data_size = 0;
        const auto send_result = xQueueSend(context.status_to_ble_queue, &status, 0);
        if (pdTRUE != send_result)
        {
            NRF_LOG_ERROR("mem: failed to send response to BLE");
        }
        return;
    }

    switch (command_id)
    {
        case ble::CommandToMemory::GET_FILES_LIST:
        {
            const auto mount_result = memory::filesystem::init_littlefs(lfs, lfs_configuration);
            if (mount_result != result::Result::OK)
            {
                NRF_LOG_ERROR("mem: failed to mount littlefs");
                status.status = ble::StatusFromMemory::ERROR_FS_CORRUPT;
                status.data_size = 0;
                break;
            }
            const auto ls_result = memory::filesystem::get_files_list(
                lfs, 
                data_queue_elem.size, 
                data_queue_elem.data, 
                ble::FileDataFromMemoryQueueElement::element_max_size);
            if (result::Result::OK != ls_result)
            {
                NRF_LOG_ERROR("mem: failed to fetch files list");
                status.status = ble::StatusFromMemory::ERROR_OTHER;
                status.data_size = 0;
            }
            break;
        }
        case ble::CommandToMemory::GET_FILE_INFO:
        {
            static constexpr uint32_t max_file_name_size{8 + 1};
            char target_file_name[max_file_name_size] = {0};
            convert_file_id_to_string(file_id, target_file_name);
            const auto file_info_result = memory::filesystem::get_file_info(
                lfs,
                target_file_name, 
                data_queue_elem.data, 
                data_queue_elem.size, 
                ble::FileDataFromMemoryQueueElement::element_max_size);

            if (result::Result::OK != file_info_result)
            {
                NRF_LOG_ERROR("mem: failed to fetch file info");
                status.status = ble::StatusFromMemory::ERROR_OTHER;
                status.data_size = 0;
            }
            if (data_queue_elem.size == 0)
            {
                NRF_LOG_ERROR("mem: file not found");
                status.status = ble::StatusFromMemory::ERROR_FILE_NOT_FOUND;
            }
            break;
        }
        default:
        {
            NRF_LOG_ERROR("mem from ble: command %d not yet implemented", command_id);
            break;
        }
    }
    const auto send_result = xQueueSend(context.status_to_ble_queue, &status, 0);
    if (pdTRUE != send_result)
    {
        NRF_LOG_ERROR("mem: failed to send status to BLE");
        return;
    }
    if (status.status != ble::StatusFromMemory::OK)
    {
        return;
    }

    const auto send_data_result = xQueueSend(context.data_to_ble_queue, &data_queue_elem, 0);
    if (pdTRUE != send_data_result)
    {
        NRF_LOG_ERROR("mem: failed to send data to BLE");
        return;
    }
}

void launch_test_1()
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

void launch_test_2()
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

void launch_test_3()
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
            NRF_LOG_ERROR("mem: failed to format LFS");
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
    NRF_LOG_INFO("mem: creating file");
    lfs_file_t file;
    const auto create_result = lfs_file_open(&lfs, &file, "rec0", LFS_O_WRONLY | LFS_O_CREAT);
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
    for (int i = 0; i < 20; ++i)
    {
        const auto write_result = lfs_file_write(&lfs, &file, test_data, sizeof(test_data));
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
    const auto close_result_1 = lfs_file_close(&lfs, &file);
    if (close_result_1!= 0)
    {
        NRF_LOG_ERROR("lfs: failed to close file after writing");
        return;
    }
    memset(test_data, 0, test_data_size);
    // ============ Step 4: open the file for read
    NRF_LOG_INFO("mem: opening file for read");
    const auto open_to_read_result = lfs_file_open(&lfs, &file, "rec0", LFS_O_RDONLY);
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

}