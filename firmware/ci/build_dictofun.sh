#!/bin/bash

if [ "$(basename "$PWD")" != "build" ]; then
    >&2 echo "you're not in \"build\" folder. Build has failed";
    exit 1
fi

rm -rf ./*

cmake -S .. -B . \
    -DCMAKE_BUILD_TYPE:STRING=Debug \
    -DBUILD_TARGET:STRING=dictofun \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi.cmake \
    -DTOOLCHAIN_PREFIX=/tools/gcc-arm-none-eabi-10.3-2021.10 \
    -DNRF5_SDK_PATH=/sdk/nRF5_SDK_17.1.0_ddde560 && \
    -DTHIRD_PARTY_PATH=/src/lib/third_party && \
    make -j
