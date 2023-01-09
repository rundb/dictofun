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

constexpr uint32_t cmd_wait_ticks{10};

void launch_test_1();
void launch_test_2();
void launch_test_3();

void task_memory(void * context_ptr)
{
    NRF_LOG_INFO("task memory: initialized");
    Context& context = *(reinterpret_cast<Context *>(context_ptr));
    CommandQueueElement command;

    flash_spi.init(flash_spi_config);
    flash.init();
    flash.reset();
    uint8_t jedec_id[6];
    flash.readJedecId(jedec_id);
    NRF_LOG_INFO("memory id: %x-%x-%x", jedec_id[0], jedec_id[1], jedec_id[2]);
    
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
        }
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

    vTaskDelay(100);

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

    vTaskDelay(100);

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

    vTaskDelay(100);

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
    
    vTaskDelay(100);

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
}