# FreeRTOS files are taken from SDK installation folder (so it's not much different from 
# other cmake modules)

# FreeRTOS common
add_library(nrf5_freertos OBJECT EXCLUDE_FROM_ALL
  "${NRF5_SDK_PATH}/external/freertos/source/croutine.c" 
  "${NRF5_SDK_PATH}/external/freertos/source/event_groups.c"
  "${NRF5_SDK_PATH}/external/freertos/source/list.c"
  "${NRF5_SDK_PATH}/external/freertos/source/queue.c"  
  "${NRF5_SDK_PATH}/external/freertos/source/stream_buffer.c"
  "${NRF5_SDK_PATH}/external/freertos/source/tasks.c"
  "${NRF5_SDK_PATH}/external/freertos/source/timers.c"
)

set(FREERTOS_CONFIG_PATH "" CACHE PATH "Path to the FreeRTOSConfig.h file. If not specified, project won't be built.")
if(NOT FREERTOS_CONFIG_PATH)
    message(FATAL_ERROR "Cannot find FreeRTOSConfig.h, please specify it via FREERTOS_CONFIG_PATH variable")
endif()

target_include_directories(nrf5_freertos PUBLIC
  "${NRF5_SDK_PATH}/external/freertos/source/include"
  "${FREERTOS_CONFIG_PATH}"
)

add_library(nrf5_freertos_interface INTERFACE)
target_include_directories(nrf5_freertos_interface INTERFACE
    "${NRF5_SDK_PATH}/external/freertos/source/include"
    "${FREERTOS_CONFIG_PATH}"
)

target_link_libraries(nrf5_freertos_interface INTERFACE
    nrf5_soc
    nrf5_app_util_platform
    nrf52_freertos_portable_gcc_interface
)

target_link_libraries(nrf5_freertos PUBLIC
    nrf5_soc
    nrf5_app_util_platform
    # TODO: consider making these dependencies conditional
    nrf52_freertos_portable_gcc
    nrf52_freertos_portable_cmsis
)

# TODO: if needed, add portable lib for memory managers 2-5
add_library(nrf5_freertos_portable_memmang_1 OBJECT EXCLUDE_FROM_ALL
    "${NRF5_SDK_PATH}/external/freertos/source/portable/MemMang/heap_1.c"
)

target_link_libraries(nrf5_freertos_portable_memmang_1 PUBLIC
    nrf5_freertos_interface
    nrf52_freertos_portable_cmsis
)

add_library(nrf52_freertos_portable_cmsis OBJECT EXCLUDE_FROM_ALL
    "${NRF5_SDK_PATH}/external/freertos/portable/CMSIS/nrf52/port_cmsis.c"
    "${NRF5_SDK_PATH}/external/freertos/portable/CMSIS/nrf52/port_cmsis_systick.c"
)

target_include_directories(nrf52_freertos_portable_cmsis PUBLIC
  "${NRF5_SDK_PATH}/external/freertos/portable/CMSIS/nrf52"
)

target_link_libraries(nrf52_freertos_portable_cmsis PUBLIC
    nrf5_freertos_interface
    nrf5_sdh
    nrf5_nrfx_hal
    nrf5_drv_clock
)

add_library(nrf52_freertos_portable_gcc OBJECT EXCLUDE_FROM_ALL
    "${NRF5_SDK_PATH}/external/freertos/portable/GCC/nrf52/port.c"
)

target_include_directories(nrf52_freertos_portable_gcc PUBLIC
  "${NRF5_SDK_PATH}/external/freertos/portable/GCC/nrf52"
)

target_link_libraries(nrf52_freertos_portable_gcc PUBLIC
    nrf5_freertos_interface
    nrf52_freertos_portable_cmsis
)

add_library(nrf52_freertos_portable_gcc_interface INTERFACE)
target_include_directories(nrf52_freertos_portable_gcc_interface INTERFACE
    "${NRF5_SDK_PATH}/external/freertos/portable/GCC/nrf52"
)
