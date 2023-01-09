add_library(littlefs OBJECT
    "${THIRD_PARTY_PATH}/littlefs/lfs.c"
    "${THIRD_PARTY_PATH}/littlefs/lfs_util.c"
)

target_include_directories(littlefs PUBLIC "${THIRD_PARTY_PATH}/littlefs")
