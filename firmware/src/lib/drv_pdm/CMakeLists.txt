add_library(drv_pdm STATIC
    ./drv_audio.c
)

target_include_directories(drv_pdm PUBLIC ./)

target_link_libraries(drv_pdm PUBLIC 
    boards
    nrf5_assert
    nrf5_nrfx_hal
    nrf5_app_scheduler
    nrf5_log
)


# This dependency is here to introduce include path to nrf_drv_pdm.h, which is not now present in CMake files
target_link_libraries(drv_pdm PRIVATE nrf5_drv_i2s)