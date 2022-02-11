// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2022, Roman Turkin
 */
#include "simple_fs.h"
#include <cstring>

namespace integration
{
static const size_t SPI_FLASH_PAGE_SIZE = 256;
static const size_t SPI_FLASH_SECTOR_SIZE = 4096;

result::Result
spi_flash_simple_fs_api_read(const uint32_t address, uint8_t* data, const size_t size)
{
    return result::Result::ERROR_NOT_IMPLEMENTED;
}

result::Result
spi_flash_simple_fs_api_write(const uint32_t address, const uint8_t* const data, const size_t size)
{
    return result::Result::ERROR_NOT_IMPLEMENTED;
}

result::Result spi_flash_simple_fs_api_erase(const uint32_t address, const size_t size)
{
    return result::Result::ERROR_NOT_IMPLEMENTED;
}

filesystem::SpiFlashConfiguration spi_flash_simple_fs_config = {spi_flash_simple_fs_api_read,
                                                                spi_flash_simple_fs_api_write,
                                                                spi_flash_simple_fs_api_erase,
                                                                SPI_FLASH_PAGE_SIZE,
                                                                SPI_FLASH_SECTOR_SIZE};
} // namespace integration