#pragma once

#include "lfs.h"

// This is an attempt to introduce a filesystem that would resolve issues caused by usage of LFS.
// In particular, it should make sure that record start time is faster than linear relative to files count
// I will do my best to reuse LFS interfaces.
namespace filesystem
{

// int lfs_format(lfs_t *lfs, const struct lfs_config *config);
// int lfs_mount(lfs_t *lfs, const struct lfs_config *config);
// int lfs_unmount(lfs_t *lfs);
// lfs_ssize_t lfs_file_read(lfs_t *lfs, lfs_file_t *file, void *buffer, lfs_size_t size);
// lfs_dir_read
// lfs_stat
// lfs_file_opencfg
// lfs_file_read
// lfs_file_close
// lfs_file_write

}
