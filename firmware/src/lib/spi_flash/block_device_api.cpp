// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */
#include "block_device_api.h"
#include "simple_fs.h"
#include "spi_flash.h"
#include <cstring>

namespace integration
{
static const size_t SPI_FLASH_PAGE_SIZE = 256;
static const size_t SPI_FLASH_SECTOR_SIZE = 4096;

static flash::SpiFlash* _flash{nullptr};

void init_filesystem(flash::SpiFlash* spiFlash)
{
    _flash = spiFlash;
}

result::Result
spi_flash_simple_fs_api_read(const uint32_t address, uint8_t* data, const size_t size)
{
    // todo: add paramter validation
    const auto result = _flash->read(address, data, size);
    if(result != flash::SpiFlash::Result::OK)
    {
        return result::Result::ERROR_GENERAL;
    }
    while(_flash->isBusy())
        ;
    return result::Result::OK;
}

result::Result
spi_flash_simple_fs_api_write(const uint32_t address, const uint8_t* const data, const size_t size)
{
    // todo: proceed with validation and split operation into chunks of <PAGE_SIZE> bytes
    if(size <= SPI_FLASH_PAGE_SIZE)
    {
        const auto result = _flash->program(address, data, size);
        if(result != flash::SpiFlash::Result::OK)
        {
            return result::Result::ERROR_GENERAL;
        }
        while(_flash->isBusy())
            ;
        return result::Result::OK;
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
        while(_flash->isBusy())
            ;
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