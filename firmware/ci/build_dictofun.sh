#!/bin/bash

cmake -S .. -B . -DCMAKE_BUILD_TYPE:STRING=Debug -DBUILD_TARGET:STRING=dictofun -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi.cmake -DTOOLCHAIN_PREFIX=/tools/gcc-arm-none-eabi-10.3-2021.10 && make -j