#!/bin/bash

FOLDER_NAME="nrf5_sdk_17.1.0_ddde560"
FILE_NAME="$FOLDER_NAME.zip"

wget "https://nsscprodmedia.blob.core.windows.net/prod/software-and-other-downloads/sdks/nrf5/binaries/${FILE_NAME}"

unzip $FILE_NAME -d ./

rm $FILE_NAME

# Step 2: apply patches, if any

patch "./nRF5_SDK_17.1.0_ddde560/components/softdevice/common/nrf_sdh_freertos.c" < "sdh_stack_size_increase.patch"
patch "./nRF5_SDK_17.1.0_ddde560/components/softdevice/common/nrf_sdh_freertos.c" < "nrf_sdh_pause_resume_c.patch"
patch "./nRF5_SDK_17.1.0_ddde560/components/softdevice/common/nrf_sdh_freertos.h" < "nrf_sdh_pause_resume_h.patch"
patch "./nRF5_SDK_17.1.0_ddde560/components/ble/ble_services/ble_cts_c/ble_cts_c.c" < "nrf_cts_c_reduce_log.patch"
patch "./nRF5_SDK_17.1.0_ddde560/components/ble/nrf_ble_gq/nrf_ble_gq.c" < "nrf_gq_reduce_logs.patch"
patch -p1 --ignore-whitespace --binary < "makefile_posix.patch"

# # Step 2: install uECC (for the bootloader, uncomment when this functionality is enabled)

cd "nRF5_SDK_17.1.0_ddde560/external/micro-ecc/"

git clone https://github.com/kmackay/micro-ecc

cd nrf52hf_armgcc/armgcc

make

# If at this stage you get an error - you have to go to the Makefile.posix file specified in the error and 
# update armgcc paths for your system

