add_library(drv_pdm STATIC EXCLUDE_FROM_ALL
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

target_link_libraries(drv_pdm PRIVATE 
    nrf5_nrfx_pdm
)
