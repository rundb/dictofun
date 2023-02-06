#!/bin/bash

# step 1: flash softdevice and application
# TODO: move to pynrfjprog
nrfjprog --reset --program ../sdk/nRF5_SDK_17.1.0_ddde560/components/softdevice/s132/hex/s132_nrf52_7.2.0_softdevice.hex  --family NRF52 --chiperase --verify
if [ $? -ne 0 ]; then
    echo "Flashing SD has failed. Aborting"
    exit -1
fi

nrfjprog --reset --program ../build/src/targets/dictofun/Dictofun.hex --sectorerase --family NRF52 --verify
if [ $? -ne 0 ]; then
    echo "Flashing app has failed. Aborting"
    exit -1
fi

python3 dictofun_tests.py
if [ $? -ne 0 ]; then
    echo "Test execution has failed (dictofun)"
    exit -1
fi

python3 fts_tests.py
if [ $? -ne 0 ]; then
    echo "Test execution has failed (FTS)"
    exit -1
fi


exit 0
