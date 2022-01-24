#!/bin/bash

# Usage: package_image.sh <target_image_name>.hex
# Afterwards image can be flashed using nrfjprog: 
#     nrfjprog --program image.hex --chiperase --reset

# TODO: implement application/settings/bootloader versioning (possibly in CI)

APPLICATION_VERSION="1"
BOOTLOADER_VERSION="1"
SETTINGS_VERSION="1"

APPLICATION_IMAGE_NAME=$1
BOOTLOADER_IMAGE_NAME="bootloader.hex"
SETTINGS_FILE_NAME="settings.hex"
SOFTDEVICE_PATH="../sdk/nRF5_SDK_17.1.0_ddde560/components/softdevice/s132/hex/s132_nrf52_7.2.0_softdevice.hex"
RELEASE_KEY_PATH="../../keys/private.pem"

INTERMEDIATE_IMAGE_NAME="merged_app.hex"

FINAL_IMAGE_NAME="image.hex"
DFU_IMAGE_NAME="dfu_image.zip"

nrfutil settings generate --family NRF52 --application ${APPLICATION_IMAGE_NAME} --application-version ${APPLICATION_VERSION} --bl-settings-version ${SETTINGS_VERSION} --bootloader-version ${BOOTLOADER_VERSION} ${SETTINGS_FILE_NAME}
mergehex -m ${APPLICATION_IMAGE_NAME} ${SOFTDEVICE_PATH} ${BOOTLOADER_IMAGE_NAME} -o ${INTERMEDIATE_IMAGE_NAME}
mergehex -m merged_app.hex ${SETTINGS_FILE_NAME} -o ${FINAL_IMAGE_NAME}

rm ${INTERMEDIATE_IMAGE_NAME}
rm ${SETTINGS_FILE_NAME}

# Generate image for DFU update
nrfutil pkg generate --hw-version 52 --sd-req 0x0101 --application-version ${APPLICATION_VERSION} --application ${APPLICATION_IMAGE_NAME} --key-file ${RELEASE_KEY_PATH} ${DFU_IMAGE_NAME}
