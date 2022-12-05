// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */
#include "block_device_api.h"
#include "simple_fs.h"
#include "spi_flash.h"
#include <cstring>

#define FLASH_IS_BUSY_TIMEOUT  (10000UL)

namespace integration
{
static const size_t SPI_FLASH_PAGE_SIZE = 256;
static const size_t SPI_FLASH_SECTOR_SIZE = 4096;

static flash::SpiFlash* _flash{nullptr};

uint8_t spi_flash_jedec_id[6U]{0U};
bool is_spi_flash_operational{true};

result::Result init_filesystem(flash::SpiFlash* spiFlash)
{
    _flash = spiFlash;
    return result::Result::OK;
}

result::Result
spi_flash_simple_fs_api_read(const uint32_t address, uint8_t* data, const size_t size)
{
    if (!is_spi_flash_operational) return result::Result::ERROR_GENERAL;
    // todo: add paramter validation
    const auto result = _flash->read(address, data, size);
    if(result != flash::SpiFlash::Result::OK)
    {
        return result::Result::ERROR_GENERAL;
    }
    volatile uint32_t timeout{FLASH_IS_BUSY_TIMEOUT};
    while(_flash->isBusy() && timeout > 0U)
    {
        --timeout;
    }
    return (timeout == 0) ? result::Result::ERROR_GENERAL : result::Result::OK;
}

result::Result
spi_flash_simple_fs_api_write(const uint32_t address, const uint8_t* const data, const size_t size)
{
    if (!is_spi_flash_operational) return result::Result::ERROR_GENERAL;
    // todo: proceed with validation and split operation into chunks of <PAGE_SIZE> bytes
    if(size <= SPI_FLASH_PAGE_SIZE)
    {
        const auto result = _flash->program(address, data, size);
        if(result != flash::SpiFlash::Result::OK)
        {
            return result::Result::ERROR_GENERAL;
        }
        volatile uint32_t timeout{FLASH_IS_BUSY_TIMEOUT};
        while(_flash->isBusy() && timeout > 0U)
        {
            --timeout;
        }
        return (timeout == 0) ? result::Result::ERROR_GENERAL : result::Result::OK;
    }
    if(size % SPI_FLASH_PAGE_SIZE != 0)
    {
        return result::Result::ERROR_NOT_IMPLEMENTED;
    }
    for(int i = 0; i < size / SPI_FLASH_PAGE_SIZE; ++i)
    {
        const auto offset = i * SPI_FLASH_PAGE_SIZE;
        const auto result = _flash->program(address + offset, &data[offset], SPI_FLASH_PAGE_SIZE);
        if(result != flash::SpiFlash::Result::OK)
        {
            return result::Result::ERROR_GENERAL;
        }
        uint32_t timeout{FLASH_IS_BUSY_TIMEOUT};
        while(_flash->isBusy() && timeout > 0U)
        {
            --timeout;
        }
        return (timeout == 0) ? result::Result::ERROR_GENERAL : result::Result::OK;
    }
    return result::Result::OK;
}

result::Result spi_flash_simple_fs_api_erase(const uint32_t address, const size_t size)
{
    return result::Result::ERROR_NOT_IMPLEMENTED;
}

const filesystem::SpiFlashConfiguration spi_flash_simple_fs_config = {spi_flash_simple_fs_api_read,
                                                                spi_flash_simple_fs_api_write,
                                                                spi_flash_simple_fs_api_erase,
                                                                SPI_FLASH_PAGE_SIZE,
                                                                SPI_FLASH_SECTOR_SIZE};
} // namespace integration