#pragma once

#include <cstdint>

using myfs_block_t = uint32_t;
using myfs_off_t = uint32_t;
using myfs_size_t = uint32_t;

// This is an attempt to introduce a filesystem that would resolve issues caused by usage of LFS.
// In particular, it should make sure that record start time is faster than linear relative to files count
// I will do my best to reuse LFS interfaces.
namespace filesystem
{

struct myfs_t
{
    bool is_mounted{false};
    uint32_t files_count{0};
};

static constexpr uint32_t global_magic_value{0x2A7B3D1FUL};
static constexpr uint32_t file_magic_value{0xE9C864A7};
static constexpr uint32_t single_file_descriptor_size_bytes{32};
static constexpr uint32_t myfs_format_marker_size{single_file_descriptor_size_bytes};

struct myfs_config {
    void *context;
    int (*read)(const struct myfs_config *c, myfs_block_t block, myfs_off_t off, void *buffer, myfs_size_t size);
    int (*prog)(const struct myfs_config *c, myfs_block_t block, myfs_off_t off, const void *buffer, myfs_size_t size);
    int (*erase)(const struct myfs_config *c, myfs_block_t block);
    int (*sync)(const struct myfs_config *c);

    myfs_size_t read_size;
    myfs_size_t prog_size;
    myfs_size_t block_size;
    myfs_size_t block_count;
    int32_t block_cycles;

    myfs_size_t cache_size;
    myfs_size_t lookahead_size;

    void *read_buffer;
    void *prog_buffer;
    void *lookahead_buffer;

    myfs_size_t name_max;
    myfs_size_t file_max;
    myfs_size_t attr_max;
    myfs_size_t metadata_max;
};

int myfs_format(myfs_t *myfs, myfs_config *config);
int myfs_mount(myfs_t *lfs, const myfs_config *config);
// int lfs_unmount(lfs_t *lfs);
// lfs_ssize_t lfs_file_read(lfs_t *lfs, lfs_file_t *file, void *buffer, lfs_size_t size);
// lfs_dir_read
// lfs_stat
// lfs_file_opencfg
// lfs_file_read
// lfs_file_close
// lfs_file_write

}
