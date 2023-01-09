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
    
    while(1)
    {
        const auto cmd_queue_receive_status = xQueueReceive(
            context.command_queue,
            reinterpret_cast<void *>(&command),
            cmd_wait_ticks);
        if (pdPASS == cmd_queue_receive_status)
        {
            NRF_LOG_INFO("task memory: received command %d", command.command_id);
        }
    }
}

}