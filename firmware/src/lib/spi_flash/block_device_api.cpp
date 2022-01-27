#include "lfs.h"
#include "block_device_api.h"
#include "spi_flash.h"

static const uint32_t BLOCK_SIZE = 256;
static const uint32_t SECTOR_SIZE = 4096;

int spi_flash_block_device_read(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, void *buffer, lfs_size_t size)
{
    const auto address = (block * BLOCK_SIZE) + off;
    auto& flashmem = flash::SpiFlash::getInstance();
    const auto res = flashmem.read(address, static_cast<uint8_t*>(buffer), size);
    if (res != flash::SpiFlash::Result::OK)
    {
        return -1;
    }
    while (flashmem.isBusy());

    return 0;
}

int spi_flash_block_device_prog(const struct lfs_config *c, lfs_block_t block,
        lfs_off_t off, const void *buffer, lfs_size_t size)
{
    const auto address = (block * BLOCK_SIZE) + off;
    auto& flashmem = flash::SpiFlash::getInstance();
    const auto res = flashmem.program(address, static_cast<const uint8_t *>(buffer), size);
    if (res != flash::SpiFlash::Result::OK)
    {
        return -1;
    }
    
    while (flashmem.isBusy());

    return 0;
}

int spi_flash_block_device_erase(const struct lfs_config *c, lfs_block_t block)
{
    const auto address = (block * SECTOR_SIZE);
    auto& flashmem = flash::SpiFlash::getInstance();
    const auto res = flashmem.eraseSector(address);
    if (res != flash::SpiFlash::Result::OK)
    {
        return -1;
    }
    while (flashmem.isBusy());

    return 0;
}

int spi_flash_block_device_sync(const struct lfs_config *c)
{
    return 0;
}

// configuration of the filesystem is provided by this struct
const struct lfs_config lfs_configuration = {
    // block device operations
    .read  = spi_flash_block_device_read,
    .prog  = spi_flash_block_device_prog,
    .erase = spi_flash_block_device_erase,
    .sync  = spi_flash_block_device_sync,

    // block device configuration
    .read_size = 16,
    .prog_size = BLOCK_SIZE,
    .block_size = SECTOR_SIZE,
    .block_count = 4096,
    .block_cycles = 500,
    .cache_size = BLOCK_SIZE,
    .lookahead_size = 16,
};
