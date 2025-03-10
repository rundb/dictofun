# This file contains definitions that are specific for Dictofun target

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNRF_DFU_SETTINGS_VERSION=2 -DCONFIG_NFCT_PINS_AS_GPIOS -DDEBUG")
set(NRF5_BOARD "dictofun_v_1_4") 
set(NRF5_SOFTDEVICE_VARIANT "s140")
set(NRF5_LINKER_SCRIPT "${PROJECT_SOURCE_DIR}/src/targets/dictofun/dictofun_linkerscript.ld")
set(NRF5_SDKCONFIG_PATH "${PROJECT_SOURCE_DIR}/src/targets/dictofun/config")
set(FREERTOS_CONFIG_PATH "${PROJECT_SOURCE_DIR}/src/targets/dictofun/config")
set(NRF5_STACK_SIZE "8192")
set(NRF5_TARGET_FAMILY "nrf52")
