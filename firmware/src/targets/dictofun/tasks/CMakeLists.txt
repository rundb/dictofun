add_library(tasks STATIC
        battery_measurement.cpp  
        battery_measurement.h  
        task_audio.cpp  
        task_audio.h  
        task_led.cpp  
        task_led.h  
        task_memory.cpp  
        task_memory.h  
        task_state.cpp  
        task_state.h
    )

target_include_directories(tasks PUBLIC ./)

target_link_libraries(tasks PRIVATE ble_fts)
target_link_libraries(tasks PRIVATE nrf5_nrfx_saadc)