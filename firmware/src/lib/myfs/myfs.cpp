#include "myfs.h"

#include <cstring>

#include "nrf_log.h"

#include <cstdio>

#include "FreeRTOS.h"
#include "task.h"

namespace filesystem
{

/// This filesystem shall be based on a simple table at the first memory sector, containing a sequence of 
/// file descriptors.
/// File descriptor structure can be found at the structure declaration.
///
/// All files shall be aligned by page size.
/// First sector (4096 bytes at the development start) shall be fully devoted to files table ((4096 / 32) - 1 => up to 128 files per single FS instance).


/// @brief erase the flash memory that belongs to the FS instance and introduce a FS marker at the first word.
/// @return 0, if operation was successful, error code otherwise (TODO: change to optional/result type)
int myfs_format(myfs_t *myfs, const myfs_config *config)
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

// Check if filesystem is valid. 
// If it is, calculate count of existing on FS files and prepare the next writable file address
int myfs_mount(myfs_t *myfs, const myfs_config *config)
{
    if (myfs == nullptr || config == nullptr)
    {
        NRF_LOG_ERROR("mount: wrong parameter");
        return -1;
    }
    if (myfs->is_mounted)
    {
        NRF_LOG_ERROR("mount: already mounted");
        return -1;
    }
    // 0. get the FS start address from the config (TODO: place it into the config)
    myfs->fs_start_address = 0;
    static constexpr uint32_t local_buffer_size{256};
    uint8_t tmp[local_buffer_size];

    // 1. check if the marker is in place
    const auto read_result = config->read(config, myfs->fs_start_address, 0 , tmp, local_buffer_size);
    if (read_result != 0)
    {
        NRF_LOG_ERROR("mount: read operation has failed");
        return -1;
    }

    uint32_t marker{0};
    memcpy(&marker, tmp, sizeof(marker));

    if (marker != global_magic_value)
    {
        NRF_LOG_ERROR("mount: myfs marker is not found, formatting is required (0x%x != 0x%x)", marker, global_magic_value);
        return -1;
    }

    bool is_list_end_found{false};
    uint32_t current_buffer_read_address{myfs->fs_start_address};
    // offset is relative to the start of the read buffer
    uint32_t current_description_offset{single_file_descriptor_size_bytes};
    myfs_file_descriptor prev_d;
    bool is_prev_descriptor_valid{false};
    while (!is_list_end_found)
    {
        myfs_file_descriptor d;
        if (current_description_offset >= local_buffer_size)
        {
            current_buffer_read_address += local_buffer_size;
            const auto read_result_2 = config->read(
                config, 
                current_buffer_read_address / config->block_size, 
                current_buffer_read_address % config->block_size, 
                tmp, 
                local_buffer_size);
            if (read_result_2 != 0)
            {
                NRF_LOG_ERROR("mount: read operation during list evaluation has failed");
                return -1;
            }
            current_description_offset = 0;
        }
        memcpy(&d, &tmp[current_description_offset], single_file_descriptor_size_bytes);
        if (d.magic == empty_word_value)
        {
            // the next location for the new file has been discovered
            // TODO: check if previous write has been completed.
            if (is_prev_descriptor_valid)
            {
                const auto next_file_start_address = prev_d.start_address + prev_d.file_size; // todo: pad it to the %256==0
                myfs->files_count = ((current_buffer_read_address - (myfs->fs_start_address)) + current_description_offset) / single_file_descriptor_size_bytes;
                // TODO: it is set off by 1 or 2, check it at the first round of tests
                NRF_LOG_INFO("found files end, files_count=%d (LIKELY SET OFF BY 1 OR 2!!!)", myfs->files_count);
                myfs->next_file_start_address = next_file_start_address;
                myfs->is_mounted = true;
                return 0;
            }
            else 
            {
                // case of the very first existing file
                myfs->files_count = 0;
                myfs->next_file_start_address = first_file_start_location;
                myfs->next_file_descriptor_address = single_file_descriptor_size_bytes;
                NRF_LOG_INFO("mount: no files found, setting count to %d and start to %d", myfs->files_count, myfs->next_file_start_address);
                return 0;
            }

        }
        else if (d.magic != file_magic_value)
        {
            // FS is corrupt and needs formatting
            NRF_LOG_ERROR("mount: file magic value is not found");
            return -1;
        }

        prev_d = d;
        is_prev_descriptor_valid = true;
        current_description_offset += single_file_descriptor_size_bytes;
    }


    return -1;
}

void print_buffer(uint8_t * buf, uint32_t size)
{
    NRF_LOG_INFO("memory dump (%d bytes)", size);
    
    static constexpr uint32_t single_print_size{16};
    char str[9 + single_print_size * 3 + 2]{0};
    for (auto i = 0; i < size; i += single_print_size)
    {
        snprintf(str, sizeof(str), "%08x: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
            i,
            buf[i+0], buf[i+1], buf[i+2], buf[i+3], buf[i+4], buf[i+5], buf[i+6], buf[i+7],
            buf[i+8], buf[i+9], buf[i+10], buf[i+11], buf[i+12], buf[i+13], buf[i+14], buf[i+15]
        );
        NRF_LOG_INFO("%s", str);
        vTaskDelay(50);
    }
}

int myfs_file_open(myfs_t *myfs, const myfs_config& config, myfs_file_t& file, uint8_t * file_id, uint8_t flags)
{
    if (nullptr == myfs || nullptr == file_id)
    {
        NRF_LOG_ERROR("myfs open: bad input");
        return -1;
    }
    if (myfs->is_file_open)
    {
        NRF_LOG_ERROR("myfs open: file is open already");
        return -1;
    }

    if ((flags & MYFS_CREATE_FLAG) > 0)
    {
        // at this point all checks have passed
        // 1. prepare a descriptor
        myfs_file_descriptor d;
        d.magic = file_magic_value;
        d.start_address = myfs->next_file_start_address;
        memcpy(d.file_id, file_id, myfs_file_descriptor::file_id_size);
        d.file_size = empty_word_value;
        memset(d.reserved, 0xFF, sizeof(d.reserved));
        
        // 2. flash needed part of the descriptor into the table
        const auto new_file_descriptor_address = myfs->next_file_descriptor_address;
        const auto descriptor_page_start = (new_file_descriptor_address / page_size) * page_size;
        const auto descriptor_page_offset = new_file_descriptor_address - descriptor_page_start;
        NRF_LOG_INFO("descriptor start 0x%x, offset 0x%x", descriptor_page_start, descriptor_page_offset);
        const auto read_result = config.read(
            &config, 
            descriptor_page_start / config.block_size,
            descriptor_page_start % config.block_size,
            config.read_buffer,
            page_size
        );
        if (read_result != 0)
        {
            NRF_LOG_ERROR("myfs: read op failed");
            return -1;
        }
        print_buffer((uint8_t *)config.read_buffer, page_size);
        memcpy(config.read_buffer + descriptor_page_offset, &d, sizeof(d));
        print_buffer((uint8_t *)config.read_buffer, page_size);

    }


    // 3. fill in required data into the file object

    return -1;
}

}