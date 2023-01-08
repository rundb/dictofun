#include "spi_flash_simulation_api.h"

namespace memory 
{
namespace blockdevice 
{
namespace sim 
{

constexpr size_t memory_size{1024*1024*16 - 4096};
uint8_t mem[memory_size];

constexpr size_t erased_sectors_bitmap_size{4096};
uint8_t erased_sectors_bitmap[erased_sectors_bitmap_size];

void reset()
{
    memset(mem, 0xFFFFFFFF, memory_size);
    memset(erased_sectors_bitmap, 0xFFFFFFFF, erased_sectors_bitmap_size);
}

bool is_sector_erased(size_t sector)
{
    const auto byte = sector / 8;
    const auto bit = sector % 8;
    return (erased_sectors_bitmap[byte] & (1 << bit)) > 0;
}

void mark_sector_as_needs_erase(size_t sector)
{
    const auto byte = sector / 8;
    const auto bit = sector % 8;
    erased_sectors_bitmap[byte] &= (~(1 << bit)) & 0xFF;
}
void mark_sector_as_erased(size_t sector)
{
    const auto byte = sector / 8;
    const auto bit = sector % 8;
    erased_sectors_bitmap[byte] |= (1 << bit);
}

int read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    memcpy(buffer, &mem[block * sector_size + off], size);
    return 0;
}

int program(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
    memcpy(&mem[block * sector_size + off], buffer, size);
    mark_sector_as_needs_erase(block);
    return 0;
}

int erase(const struct lfs_config *c, lfs_block_t block)
{
    if (is_sector_erased(block)) return 0;

    memset(&mem[block * sector_size], 0xFFFFFFFF, sector_size);
    mark_sector_as_erased(block);
    return 0;
}

int sync(const struct lfs_config *c)
{
    return 0;
}

}
}
}