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
            // NRF_LOG_INFO("task memory: received command %d", command.command_id);
            if (command.command_id == Command::LAUNCH_TEST_1)
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
        }
    }
}

}