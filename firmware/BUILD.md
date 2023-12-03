# Build and Flash commands

Collection of commands that can be used to build the project elements

## Build commands

All build commands are launched from `<PROJECT_PATH>/firmware/build`

#### Bootloader

```
make -C ../src/targets/bootloader/pca10040_s132_ble_debug/armgcc
```
#### Application
```
cmake -S .. -B . -DCMAKE_BUILD_TYPE:STRING=Debug -DBUILD_TARGET:STRING=dictofun -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi.cmake -DNRF5_SDK_PATH=../sdk/nRF5_SDK_17.1.0_ddde560 -DTOOLCHAIN_PREFIX=/usr/share/gcc-arm-none-eabi-10.3-2021.10 && make -j
```

#### Package Application
```
nrfutil pkg generate  --hw-version 52 --sd-req 0x0101 --application-version 1 --application src/targets/dictofun/Dictofun.hex  --key-file ${DICTOFUN_PK_PATH} app_dfu_package.zip
```

## Flash commands

#### Erase the whole chip
```
openocd -f interface/cmsis-dap.cfg -f target/nrf52.cfg -c "init; reset halt; nrf5 mass_erase; reset run; exit"
```

#### Reset
```
openocd -f interface/cmsis-dap.cfg -f target/nrf52.cfg -c "init ; reset run; exit" 
```

#### Flash SoftDevice
```
openocd -f interface/cmsis-dap.cfg -f target/nrf52.cfg -c "program ../sdk/nRF5_SDK_17.1.0_ddde560/components/softdevice/s132/hex/s132_nrf52_7.2.0_softdevice.hex verify reset exit"
```

#### Flash Bootloader
```
openocd -f interface/cmsis-dap.cfg -f target/nrf52.cfg -c "program ../src/targets/bootloader/pca10040_s132_ble_debug/armgcc/_build/nrf52832_xxaa_s132.hex verify reset exit"
```