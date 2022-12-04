#!/bin/bash

FOLDER_NAME="nrf5_sdk_17.1.0_ddde560"
FILE_NAME="$FOLDER_NAME.zip"

#wget "https://www.nordicsemi.com/-/media/Software-and-other-downloads/SDKs/nRF5/Binaries/${FILE_NAME}"
wget "https://nsscprodmedia.blob.core.windows.net/prod/software-and-other-downloads/sdks/nrf5/binaries/${FILE_NAME}"

unzip $FILE_NAME -d ./

rm $FILE_NAME

# Step 2: install uECC (for the bootloader)

cd "${FOLDER_NAME}/external/micro-ecc/micro-ecc"

git clone https://github.com/kmackay/micro-ecc

cd nrf52hf_armgcc/armgcc

make

# If at this stage you get an error - you have to go to the Makefile.posix file specified in the error and 
# update armgcc paths for your system

