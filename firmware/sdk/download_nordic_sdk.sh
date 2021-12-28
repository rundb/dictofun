#!/bin/bash

FOLDER_NAME="nRF5_SDK_17.1.0_ddde560"
FILE_NAME="$FOLDER_NAME.zip"

wget "https://www.nordicsemi.com/-/media/Software-and-other-downloads/SDKs/nRF5/Binaries/${FILE_NAME}"

unzip $FILE_NAME -d ./

rm $FILE_NAME

