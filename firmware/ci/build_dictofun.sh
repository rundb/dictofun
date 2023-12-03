#!/bin/bash

if [ "$(basename "$PWD")" != "build" ]; then
    >&2 echo "you're not in \"build\" folder. Build has failed";
    exit 1
fi

rm -rf ./*


# Apply patches on the SDK, if not done so yet
patch "/sdk/nRF5_SDK_17.1.0_ddde560/components/softdevice/common/nrf_sdh_freertos.c" < "../sdk/sdh_stack_size_increase.patch" \
  && patch "/sdk/nRF5_SDK_17.1.0_ddde560/components/softdevice/common/nrf_sdh_freertos.c" < "../sdk/nrf_sdh_pause_resume_c.patch" \
  && patch "/sdk/nRF5_SDK_17.1.0_ddde560/components/softdevice/common/nrf_sdh_freertos.h" < "../sdk/nrf_sdh_pause_resume_h.patch" \
  && patch "/sdk/nRF5_SDK_17.1.0_ddde560/components/ble/ble_services/ble_cts_c/ble_cts_c.c" < "../sdk/nrf_cts_c_reduce_log.patch" \
  && patch "/sdk/nRF5_SDK_17.1.0_ddde560/components/ble/nrf_ble_gq/nrf_ble_gq.c" < "../sdk/nrf_gq_reduce_logs.patch"


cmake -S .. -B . \
    -DCMAKE_BUILD_TYPE:STRING=Debug \
    -DBUILD_TARGET:STRING=dictofun \
    -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi.cmake \
    -DTOOLCHAIN_PREFIX=/tools/gcc-arm-none-eabi-10.3-2021.10 \
    -DNRF5_SDK_PATH=/sdk/nRF5_SDK_17.1.0_ddde560 && \
    make -j
