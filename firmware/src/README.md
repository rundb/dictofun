# Firmware

This folder contains source code related to firmware for `dictofun` hardware.

## Code structure

Following code structure is intended:

- `../sdk/nRF5_SDK_17.1.0_ddde560` - path to extracted Nordic SDK
- `lib` contains modules that do not necessarily stick to a particular hardware. 
- `targets` contains target-specific code.
- - target `dictofun` is the target application firmware that is eventually executed on the dictofun hardware
- - target `pca10040` is intended to be used with NRF52 DK. It should be used for development and testing of the modules in lib folder.
- - target `bootloader` is the bootloader code. Out goal is to minimize development of bootloader and maximal reuse and compatibility with the bootloader provided by Nordic.

## How to build

Segger Embedded Studio is currently required for building and flashing the firmware. It's use is free with NRF52 projects, including usage for commercial purposes (see https://wiki.segger.com/Get_a_License_for_Nordic_Semiconductor_Devices). It can be downloaded from here: https://www.segger.com/downloads/embedded-studio.

1) download and extract NRF52 SDK to `sdk` folder (if you use `Windows` - you can do it with WSL or manually: `https://www.nordicsemi.com/-/media/Software-and-other-downloads/SDKs/nRF5/Binaries/nRF5_SDK_17.1.0_ddde560`)

2) open file `targets/dictofun/dictofun_s132.emProject`

3) press `Build`. If you have a J-Link/NRF52 DK - you can flash the dictofun device with it. Press `Target->Connect J-Link`, then `Erase all`, then `Download dictofun_s132`.



