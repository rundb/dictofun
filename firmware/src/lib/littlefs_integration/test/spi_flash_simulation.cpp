#include "spi_flash_simulation_api.h"

namespace memory 
{
namespace blockdevice 
{
namespace sim 
{

int read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    return -1;
}

int program(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
    return -1;
}

int erase(const struct lfs_config *c, lfs_block_t block)
{
    return -1;
}

int sync(const struct lfs_config *c)
{
    return -1;
}

}
}
}