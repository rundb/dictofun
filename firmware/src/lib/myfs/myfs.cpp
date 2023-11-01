#include "myfs.h"

#include <cstring>

namespace filesystem
{

int myfs_format(myfs_t *myfs, myfs_config *config)
{
    if (nullptr == myfs || config == nullptr)
    {
        return -1;
    }

    // first erase the whole flash memory. At first should be done somewhere else, as SpiIf doesn't give this interface

    // propagate a magic value into the first 32 bytes
    uint8_t format_marker[myfs_format_marker_size];
    memset(format_marker, 0xFF, myfs_format_marker_size);
    memcpy(format_marker, &global_magic_value, sizeof(global_magic_value));
    const auto read_result = config->read(config, 0, 0, config->read_buffer, config->prog_size);
    if (read_result != 0)
    {
        return read_result;
    }
    memcpy(config->prog_buffer, config->read_buffer, config->prog_size);
    memcpy(config->prog_buffer, format_marker, sizeof(format_marker));

    const auto program_result = config->prog(config, 0, 0, config->prog_buffer, config->prog_size);
    if (program_result != 0)
    {
        return program_result;
    }
    myfs->files_count = 0;
    return 0;
}

int myfs_mount(myfs_t *lfs, const myfs_config *config)
{
    return -1;
}
}