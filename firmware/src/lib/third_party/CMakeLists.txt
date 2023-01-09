add_library(littlefs OBJECT
    "./littlefs/lfs.c"
    "./littlefs/lfs_util.c"
)

target_include_directories(littlefs PUBLIC "./littlefs")
