// SPDX-License-Identifier:  Apache-2.0
/*
 * Copyright (c) 2023, Roman Turkin
 */

#include "myfs.h"

#include <cstring>

#include <algorithm>
#include <cstdio>

namespace filesystem
{

/// This filesystem shall be based on a simple table at the first memory sector, containing a sequence of
/// file descriptors.
/// File descriptor structure can be found at the structure declaration.
///
/// All files shall be aligned by page size.
/// First sector (4096 bytes at the development start) shall be fully devoted to files table ((4096 / 32) - 1 => up to 128 files per single FS instance).

// ==================== Private functions =================
int write_myfs_descriptor(myfs_file_descriptor d,
                          const uint32_t descriptor_address,
                          const myfs_config& c);
int read_myfs_descriptor(myfs_file_descriptor& d,
                         const uint32_t descriptor_address,
                         const myfs_config& c);
void print_flash_memory_area(const myfs_config& c, uint32_t start_address, uint32_t size);

// This variable is set up at the mounting stage. It defines boundaries for the binary search algorithm.
static uint32_t max_files_in_fs{legacy_first_file_start_location / single_file_descriptor_size_bytes};

static uint32_t get_first_file_offset() { return max_files_in_fs * single_file_descriptor_size_bytes; }

/// @brief erase the flash memory that belongs to the FS instance and introduce a FS marker at the first word.
/// @return 0, if operation was successful, error code otherwise (TODO: change to optional/result type)
int myfs_format(myfs_t& myfs)
{
    const myfs_config config(myfs.config);

    const auto erase_result = config.erase_multiple(&config, 0, config.block_count);
    if (erase_result != 0)
    {
        return erase_result;
    }

    // propagate a magic value into the first 32 bytes
    uint8_t format_marker[myfs_format_marker_size];
    uint32_t fs_size{first_file_start_location / single_file_descriptor_size_bytes};
    memset(format_marker, 0xFF, myfs_format_marker_size);
    memcpy(format_marker, &global_magic_value, sizeof(global_magic_value));
    memcpy(&format_marker[4], &fs_size, sizeof(fs_size));
    const auto read_result = config.read(&config, 0, 0, config.read_buffer, config.prog_size);
    if(read_result != 0)
    {
        return read_result;
    }
    memcpy(config.prog_buffer, config.read_buffer, config.prog_size);
    memcpy(config.prog_buffer, format_marker, sizeof(format_marker));

    const auto program_result = config.prog(&config, 0, 0, config.prog_buffer, config.prog_size);
    if(program_result != 0)
    {
        return program_result;
    }
    myfs.files_count = 0;
    return 0;
}

// Check if filesystem is valid.
// If it is, calculate count of existing on FS files and prepare the next writable file address
int myfs_mount(myfs_t& myfs)
{
    const myfs_config config(myfs.config);

    if(myfs.is_mounted)
    {
        return -1;
    }
    // 0. get the FS start address from the config (TODO: place it into the config)
    myfs.fs_start_address = 0;
    static constexpr uint32_t local_buffer_size{page_size};
    uint8_t tmp[local_buffer_size];

    // 1. check if the marker is in place
    const auto read_result = config.read(&config, myfs.fs_start_address, 0, tmp, local_buffer_size);
    if(read_result != 0)
    {
        return -1;
    }

    uint32_t marker{0};
    memcpy(&marker, tmp, sizeof(marker));

    if(marker != global_magic_value)
    {
        return -1;
    }

    uint32_t fs_size{0};
    memcpy(&fs_size, &tmp[sizeof(marker)], sizeof(fs_size));

    if (fs_size > 0 && fs_size < 4096) 
    {
        max_files_in_fs = fs_size;
    }

    bool is_list_end_found{false};
    // offset is relative to the start of the read buffer
    uint32_t current_descriptor_address{myfs.fs_start_address + single_file_descriptor_size_bytes};

    uint32_t current_file_id{max_files_in_fs / 2};
    uint32_t current_step_size{current_file_id / 2};
    while (current_step_size > 0)
    {
        myfs_file_descriptor d;
        uint32_t descriptor_address = current_file_id * single_file_descriptor_size_bytes;

        const auto read_res = read_myfs_descriptor(d, descriptor_address, config);
        if(0 != read_res)
        {
            return read_res;
        }

        uint32_t next_file_id{0xFFFFEEEE}; // intentionally invalid value
        if(d.magic == empty_word_value)
        {
            next_file_id = current_file_id - current_step_size;
        }
        else if (d.magic == file_magic_value)
        {
            if (d.file_size != empty_word_value)
            {
                next_file_id = current_file_id + current_step_size;
            }
            else 
            {
                const auto repair_result = myfs_repair(myfs, d, descriptor_address);
                if (repair_result != 0)
                {
                    return repair_result;
                }
                return REPAIR_HAS_BEEN_PERFORMED;
            }
        }
        else
        {
            // FS is corrupt and needs formatting
            return -1;
        }
        current_file_id = next_file_id;
        current_step_size /= 2;
        if (current_step_size == 0)
        {
            // the last file has been found.

            // special case of the first written file
            if (current_file_id == 1 || current_file_id == 2)
            {
                myfs_file_descriptor first_d;
                myfs_file_descriptor second_d;
                uint32_t first_descriptor_address = single_file_descriptor_size_bytes;
                uint32_t second_descriptor_address = 2 * single_file_descriptor_size_bytes;
                [[maybe_unused]] const auto read_res_first = read_myfs_descriptor(first_d, first_descriptor_address, config);
                [[maybe_unused]] const auto read_res_second = read_myfs_descriptor(second_d, second_descriptor_address, config);
                if (first_d.magic == empty_word_value)
                {
                    myfs.files_count = 0;
                    myfs.next_file_start_address = get_first_file_offset();
                    myfs.next_file_descriptor_address = single_file_descriptor_size_bytes;
                }
                else if (second_d.magic == empty_word_value)
                {
                    myfs.files_count = 1;
                    const auto first_file_size = first_d.file_size;
                    myfs.next_file_start_address = get_first_file_offset() + ((first_file_size / page_size) + 1) * page_size;
                    myfs.next_file_descriptor_address = 2 * single_file_descriptor_size_bytes;
                }
                else 
                {
                    return IMPLEMENTATION_ERROR;
                }
                myfs.is_mounted = true;
                return 0;
            }
            else if (current_file_id == max_files_in_fs - 1) 
            {
                // file system is full, special case, tbd the next action
                return NO_SPACE_LEFT;
            }
            else 
            {
                // read surrounding descriptors
                myfs_file_descriptor prev_d;
                myfs_file_descriptor next_d;
                uint32_t prev_descriptor_address = (current_file_id - 1) * single_file_descriptor_size_bytes;
                uint32_t next_descriptor_address = (current_file_id + 1) * single_file_descriptor_size_bytes;
                [[maybe_unused]] const auto read_res_prev = read_myfs_descriptor(prev_d, prev_descriptor_address, config);
                [[maybe_unused]] const auto read_res_next = read_myfs_descriptor(next_d, next_descriptor_address, config);
                myfs_file_descriptor& last_written_descriptor = (next_d.magic == file_magic_value) ? next_d : 
                                                                (d.magic == file_magic_value) ? d : prev_d;
                uint32_t last_written_file_id = (next_d.magic == file_magic_value) ? (current_file_id + 1) : 
                                                (d.magic == file_magic_value) ? current_file_id : (current_file_id - 1);
                if (last_written_descriptor.file_size != empty_word_value)
                {
                    const auto last_written_file_size = last_written_descriptor.file_size;
                    const auto next_descriptor_address = (last_written_file_id + 1) * single_file_descriptor_size_bytes;
                    const auto next_file_start_address = last_written_descriptor.start_address + ((last_written_descriptor.file_size / page_size) + 1) * page_size;
                    
                    myfs.files_count = last_written_file_id;
                    myfs.next_file_start_address = next_file_start_address;
                    myfs.next_file_descriptor_address = next_descriptor_address;

                    return 0;
                }
                else 
                {
                    // probably a repair is required
                    return FS_CORRUPT_REPAIRABLE;
                }
            }
        }
        else 
        {
            current_file_id = next_file_id;
        }
    }

    return -1;
}

int myfs_file_open(myfs_t& myfs, myfs_file_t& file, uint8_t* file_id, uint8_t flags)
{
    const myfs_config config(myfs.config);

    if(myfs.is_file_open)
    {
        return -1;
    }

    if((flags & MYFS_CREATE_FLAG) > 0)
    {
        // at this point all checks have passed
        // 1. prepare a descriptor
        myfs_file_descriptor d;
        d.magic = file_magic_value;
        d.start_address = myfs.next_file_start_address;
        memcpy(d.file_id, file_id, myfs_file_descriptor::file_id_size);
        d.file_size = empty_word_value;
        memset(d.reserved, 0xFF, sizeof(d.reserved));

        // 2. flash needed part of the descriptor into the table
        const auto descr_prog_result =
            write_myfs_descriptor(d, myfs.next_file_descriptor_address, config);
        if(0 != descr_prog_result)
        {
            return -1;
        }

        // 3. fill in required data into the file object
        file.flags = flags;
        file.is_open = true;
        file.is_write = true;
        file.size = 0;
        myfs.is_file_open = true;
        myfs.buffer_pointer = reinterpret_cast<uint8_t*>(config.prog_buffer);
        myfs.buffer_size = config.prog_size;
        myfs.buffer_position = 0;
        memcpy(file.id, file_id, myfs_file_t::id_size);

        // 4. Debug printout of the configured descriptor
        // print_flash_memory_area(config, myfs.next_file_descriptor_address, single_file_descriptor_size_bytes);
        return 0;
    }
    else if((flags & MYFS_READ_FLAG) > 0)
    {
        // run through the filesystem and compare saved file IDs with the requested one
        bool is_file_found{false};
        uint32_t current_descriptor_address{myfs.fs_start_address +
                                            single_file_descriptor_size_bytes};
        while(!is_file_found)
        {
            myfs_file_descriptor d;
            const auto descr_read_result =
                read_myfs_descriptor(d, current_descriptor_address, config);
            if(0 != descr_read_result)
            {
                return -1;
            }
            if(d.magic != file_magic_value)
            {
                return -1;
            }
            if(memcmp(d.file_id, file_id, d.file_id_size) == 0)
            {
                // target file was found
                file.flags = flags;
                file.is_open = true;
                file.is_write = false;
                file.size = d.file_size;
                file.read_pos = 0;
                file.start_address = d.start_address;
                myfs.is_file_open = true;
                myfs.buffer_pointer = reinterpret_cast<uint8_t*>(config.prog_buffer);
                myfs.buffer_size = config.prog_size;
                myfs.buffer_position = 0;
                is_file_found = true;
            }
            else
            {
                current_descriptor_address += single_file_descriptor_size_bytes;
            }
        }
        if(is_file_found)
        {
            return 0;
        }
    }
    return -1;
}

// close operation updates the descriptor contained in flash memory with value of size and (maybe later) CRC value
int myfs_file_close(myfs_t& myfs, myfs_file_t& file)
{
    const myfs_config& config(myfs.config);

    if(!file.is_open)
    {
        return -1;
    }

    if(file.is_write)
    {
        myfs_file_descriptor d;
        const auto read_res = read_myfs_descriptor(d, myfs.next_file_descriptor_address, config);
        if(0 != read_res)
        {
            return read_res;
        }
        if(d.magic != file_magic_value)
        {
            return -1;
        }
        // first flush contents of the prog buffer into flash memory
        const auto prog_address = myfs.next_file_start_address + file.size;
        // should be page-aligned at this point
        if(prog_address % page_size != 0)
        {
            return -1;
        }
        const auto block = prog_address / config.block_size;
        const auto off = prog_address % config.block_size;
        memset(&myfs.buffer_pointer[myfs.buffer_position],
               0x00,
               myfs.buffer_size - myfs.buffer_position);
        const auto prog_result =
            config.prog(&config, block, off, myfs.buffer_pointer, myfs.buffer_size);
        if(prog_result != 0)
        {
            // it's not a critical error, we just lose data, but we still can proceed
            // TODO: decide on a proper informing about this event
        }
        file.size += myfs.buffer_position;

        d.file_size = file.size;
        const auto prog_res = write_myfs_descriptor(d, myfs.next_file_descriptor_address, config);
        if(0 != prog_res)
        {
            return prog_res;
        }

        const uint32_t next_descriptor_position =
            myfs.next_file_descriptor_address + single_file_descriptor_size_bytes;
        const uint32_t next_file_start_address =
            myfs.next_file_start_address + ((file.size / page_size) + 1) * page_size;

        myfs.next_file_descriptor_address = next_descriptor_position;
        myfs.next_file_start_address = next_file_start_address;
        myfs.files_count++;
        myfs.is_file_open = false;
        file.is_open = false;

        return 0;
    }
    else
    {
        file.is_open = false;
        myfs.is_file_open = false;
        return 0;
    }

    return -1;
}

int myfs_file_write(myfs_t& myfs, myfs_file_t& file, void* buffer, myfs_size_t size)
{
    const myfs_config& config(myfs.config);
    if(nullptr == buffer || !file.is_open || !file.is_write)
    {
        return INVALID_PARAMETERS;
    }
    // handle case of data fitting into the buffer
    const auto leftover_space = myfs.buffer_size - myfs.buffer_position;
    if(leftover_space > size)
    {
        memcpy(&myfs.buffer_pointer[myfs.buffer_position], buffer, size);
        myfs.buffer_position += size;
        return 0;
    }
    // fill in the buffer until full, then flush onto the disk
    memcpy(&myfs.buffer_pointer[myfs.buffer_position], buffer, leftover_space);
    const auto prog_address = myfs.next_file_start_address + file.size;
    // should be page-aligned at this point
    if(prog_address % page_size != 0)
    {
        return ALIGNMENT_ERROR;
    }
    const auto block = prog_address / config.block_size;
    const auto off = prog_address % config.block_size;
    const auto prog_result =
        config.prog(&config, block, off, myfs.buffer_pointer, myfs.buffer_size);
    if(prog_result != 0)
    {
        // it's not a critical error, we just lose data, but we still can proceed
        return INTERNAL_ERROR;
    }

    // copy the rest of the data into the temporary buffer
    const auto leftover_data_size = size - leftover_space;
    if(leftover_data_size > myfs.buffer_size)
    {
        return INVALID_PARAMETERS;
    }
    memcpy(myfs.buffer_pointer,
           &(reinterpret_cast<uint8_t*>(buffer))[leftover_space],
           leftover_data_size);

    myfs.buffer_position = leftover_data_size;

    // file size only updates together with buffer flushing and follows it's size
    file.size += myfs.buffer_size;

    return 0;
}

int myfs_file_read(
    myfs_t& myfs, myfs_file_t& file, void* buffer, myfs_size_t max_size, myfs_size_t& read_size)
{
    const myfs_config& config(myfs.config);
    if(nullptr == buffer || !file.is_open || file.is_write)
    {
        return -1;
    }
    const auto leftover_size = file.size - file.read_pos;
    if(leftover_size == 0)
    {
        read_size = 0;
        return 0;
    }
    read_size = std::min(max_size, leftover_size);

    const auto read_address = file.start_address + file.read_pos;
    const auto block = read_address / config.block_size;
    const auto off = read_address % config.block_size;
    const auto read_res = config.read(&config, block, off, buffer, read_size);
    if(read_res != 0)
    {
        return -1;
    }
    file.read_pos += read_size;

    return 0;
}

int myfs_unmount(myfs_t& myfs)
{
    myfs.is_mounted = false;
    return 0;
}

uint32_t myfs_get_files_count(myfs_t& myfs)
{
    return myfs.files_count;
}

int myfs_rewind_dir(myfs_t& myfs)
{
    if(!myfs.is_mounted)
    {
        return -1;
    }
    myfs.current_id_search_pos = single_file_descriptor_size_bytes;
    return 0;
}

int myfs_get_next_id(myfs_t& myfs, uint8_t* file_id)
{
    const myfs_config& config(myfs.config);
    if(nullptr == file_id)
    {
        return -1;
    }

    myfs_file_descriptor d;
    const auto read_res = read_myfs_descriptor(d, myfs.current_id_search_pos, config);
    if(0 != read_res)
    {
        return -1;
    }
    if(d.magic != file_magic_value)
    {
        // end of file system has been reached
        return 0;
    }
    memcpy(file_id, d.file_id, myfs_file_t::id_size);
    myfs.current_id_search_pos += single_file_descriptor_size_bytes;
    return 1;
}

// TODO: utilize binary search here
int myfs_file_get_size(myfs_t& myfs, uint8_t* file_id)
{
    const myfs_config& config(myfs.config);
    if(nullptr == file_id)
    {
        return -1;
    }
    bool is_file_found{false};
    uint32_t current_descriptor_address{myfs.fs_start_address + single_file_descriptor_size_bytes};

    while(!is_file_found)
    {
        myfs_file_descriptor d;
        const auto descr_read_result = read_myfs_descriptor(d, current_descriptor_address, config);
        if(0 != descr_read_result)
        {
            return -1;
        }
        if(d.magic != file_magic_value)
        {
            char filename[17]{0};
            snprintf(filename, sizeof(filename), "%02x%02x%02x%02x%02x%02x%02x%02x",
                file_id[0],
                file_id[1],
                file_id[2],
                file_id[3],
                file_id[4],
                file_id[5],
                file_id[6],
                file_id[7]
                );
            return ERROR_FILE_NOT_FOUND;
        }

        if(memcmp(d.file_id, file_id, d.file_id_size) == 0)
        {
            is_file_found = true;
            if(d.file_size != empty_word_value)
            {
                return d.file_size;
            }
            return -1;
        }
        current_descriptor_address += single_file_descriptor_size_bytes;
    }
    return -1;
}

int myfs_get_fs_stat(myfs_t& myfs, uint32_t& files_count, uint32_t& occupied_space)
{
    const myfs_config& config(myfs.config);
    if(!myfs.is_mounted)
    {
        return -1;
    }

    uint32_t current_descriptor_address{myfs.fs_start_address + single_file_descriptor_size_bytes};
    files_count = 0;
    occupied_space = first_file_start_location;
    while(true)
    {
        myfs_file_descriptor d;
        const auto descr_read_result = read_myfs_descriptor(d, current_descriptor_address, config);
        if(0 != descr_read_result)
        {
            return -1;
        }
        if(d.magic != file_magic_value)
        {
            return 0;
        }
        files_count++;
        if(d.file_size != empty_word_value)
        {
            occupied_space += d.file_size;
        }
        else
        {
            // TODO: make a decision on proper handling
        }
        current_descriptor_address += single_file_descriptor_size_bytes;
    }
    return 0;
}

int write_myfs_descriptor(myfs_file_descriptor d,
                          const uint32_t descriptor_address,
                          const myfs_config& c)
{
    // the whole page containing this descriptor should be read before applying changes
    uint8_t tmp[page_size];
    const auto page_address = (descriptor_address / page_size) * page_size;
    const auto block_id = page_address / c.block_size;
    const auto block_offset = page_address % c.block_size;
    const auto read_res = c.read(&c, block_id, block_offset, tmp, page_size);
    if(read_res != 0)
    {
        return read_res;
    }
    const auto descriptor_offset_to_page = descriptor_address - page_address;
    memcpy(&tmp[descriptor_offset_to_page], &d, sizeof(d));
    const auto prog_res = c.prog(&c, block_id, block_offset, tmp, page_size);

    if(prog_res != 0)
    {
        return prog_res;
    }
    return 0;
}

int read_myfs_descriptor(myfs_file_descriptor& d,
                         const uint32_t descriptor_address,
                         const myfs_config& c)
{
    const auto block_id = descriptor_address / c.block_size;
    const auto block_offset = descriptor_address % c.block_size;

    const auto read_res = c.read(&c, block_id, block_offset, &d, single_file_descriptor_size_bytes);
    if(read_res != 0)
    {
        return read_res;
    }

    return 0;
}

int myfs_repair(myfs_t& myfs, myfs_file_descriptor& first_invalid_descriptor, uint32_t descriptor_address)
{
    bool is_file_end_found{false};
    uint32_t file_size{0};
    uint32_t written_address = first_invalid_descriptor.start_address;
    static constexpr uint32_t max_files_count{256};
    const auto& c{myfs.config};
    uint32_t timeout{max_files_count};
    while (!is_file_end_found && written_address < (c.block_count * c.block_size) && --timeout > 0)
    {
        const auto block_id = written_address / c.block_size;
        const auto block_offset = written_address % c.block_size;
        uint32_t buff{0xFFFFFFFFUL};
        const auto read_res = c.read(&c, block_id, block_offset, &buff, sizeof(buff));
        if (read_res != 0)
        {
            return -1;
        }
        if (written_address == empty_word_value)
        {
            is_file_end_found = true;
        }
        else 
        {
            written_address += sizeof(buff);
            file_size += sizeof(buff);
        }
    }
    if (timeout == 0)
    {
        return -1;
    }
    if (is_file_end_found)
    {
        first_invalid_descriptor.file_size = file_size;
        const auto write_res = write_myfs_descriptor(first_invalid_descriptor, descriptor_address, c);
        return write_res;
    }

    return -1;
}

} // namespace filesystem
