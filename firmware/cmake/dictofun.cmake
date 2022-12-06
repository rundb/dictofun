# This file contains definitions that are specific for Dictofun target

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNRF_DFU_SETTINGS_VERSION=2 -DAPP_TIMER_V2 -DAPP_TIMER_V2_RTC1_ENABLED")
#set(CMAKE_TOOLCHAIN_FILE "${PROJECT_SOURCE_DIR}/cmake/arm-none-eabi.cmake")
#set(NRF5_SDK_PATH "${PROJECT_SOURCE_DIR}/sdk/nRF5_SDK_17.1.0_ddde560")
set(NRF5_BOARD "dictofun_v_1_1") 
set(NRF5_SOFTDEVICE_VARIANT "s132")
set(NRF5_LINKER_SCRIPT "${PROJECT_SOURCE_DIR}/src/targets/dictofun/dictofun_linkerscript.ld")
set(NRF5_SDKCONFIG_PATH "${PROJECT_SOURCE_DIR}/src/targets/dictofun/config")
set(FREERTOS_CONFIG_PATH "${PROJECT_SOURCE_DIR}/src/targets/dictofun/config")
