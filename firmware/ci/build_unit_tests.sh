#!/bin/bash

if [ "$(basename "$PWD")" != "build" ]; then
    >&2 echo "you're not in \"build\" folder. Build has failed";
    exit 1
fi

rm -rf ./*

cmake -S .. -B . -DBUILD_TARGET:STRING=unit-test -DTHIRD_PARTY_PATH=./src/lib/third_party && cmake --build . && ctest
