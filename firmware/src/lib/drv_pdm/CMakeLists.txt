add_library(drv_pdm STATIC
    ./drv_audio.c
)

target_include_directories(drv_pdm PUBLIC ./)
