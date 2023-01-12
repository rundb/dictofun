# This file contains definitions that are specific for Dictofun target

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNRF_DFU_SETTINGS_VERSION=2 -DCONFIG_NFCT_PINS_AS_GPIOS -DDEBUG")
set(NRF5_BOARD "dictofun_v_1_3") 
set(NRF5_SOFTDEVICE_VARIANT "s132")
set(NRF5_LINKER_SCRIPT "${PROJECT_SOURCE_DIR}/src/targets/dictofun/dictofun_linkerscript.ld")
set(NRF5_SDKCONFIG_PATH "${PROJECT_SOURCE_DIR}/src/targets/dictofun/config")
set(FREERTOS_CONFIG_PATH "${PROJECT_SOURCE_DIR}/src/targets/dictofun/config")
