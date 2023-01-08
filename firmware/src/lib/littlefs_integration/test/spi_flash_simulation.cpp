#include "spi_flash_simulation_api.h"

#include <iostream>
#include  <iomanip>
using namespace std;

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
    // cout << "\tread(block=" << block << ", off = " << off << ", size = " << size << ")" << endl;
    memcpy(buffer, &mem[block * sector_size + off], size);
    return 0;
}

int program(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
    // cout << "\tprogram(block=" << block << ", off = " << off << ", size = " << size << ")" << endl;
    memcpy(&mem[block * sector_size + off], buffer, size);
    mark_sector_as_needs_erase(block);
    return 0;
}

int erase(const struct lfs_config *c, lfs_block_t block)
{
    if (is_sector_erased(block)) return 0;

    cout << "erase(block=" << block << ")" << endl;
    memset(&mem[block * sector_size], 0xFFFFFFFF, sector_size);
    mark_sector_as_erased(block);
    return 0;
}

int sync(const struct lfs_config *c)
{
    // cout << "sync" << endl;
    return 0;
}

void display_erased_status_bitmap()
{
    constexpr size_t row_size{64};
    constexpr size_t columns_count{erased_sectors_bitmap_size / row_size};
    for (int i = 0; i < columns_count; ++i)
    {
        for (int j = 0; j < row_size; ++j)
        {
            cout << setfill('0') << setw(2) << right << hex  << (int)erased_sectors_bitmap[i * row_size + j];
        }
        cout << endl;
    }
    cout << endl;
}

void display()
{
    display_erased_status_bitmap();
    constexpr size_t row_size{64};
    constexpr size_t columns_count{256};
    for (int i = 0; i < columns_count; ++i)
    {
        if (((i * row_size) % sector_size) == 0)
        {
            cout << (i * row_size) << endl;
        }
        for (int j = 0; j < row_size; ++j)
        {
            cout << setfill('0') << setw(2) << right << hex  << (int)mem[i * row_size + j] << " ";
        }
        cout << endl;
    }
}

}
}
}