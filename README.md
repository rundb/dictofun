# DictoFun

![build](https://github.com/rundb/dictofun/actions/workflows/build-dictofun-target.yml/badge.svg)

Small wearable NRF52832-based-based voice recorder.

![DictoFun](https://github.com/rundb/dictofun/blob/master/pcb/out/v1.1/dictofun_1.1_top.PNG?raw=true)

## Concept

Device was initiated after my frustration while using voice recorder for storing ideas. It was taking up to 7 seconds from taking the voice recorder of the pocket to start recording the data. 

Idea behind the device: press and hold the big button on the device and start speaking. The device should start recording _immediately_ (== under human reaction time, <=0.4 sec), without the need for any other presses or for looking at the screen. When you're done - release the button. This should end the record. 

After finishing the record device should transfer the record to the phone. The record is then transferred to the voice recognition service. The recognized text should then be sent to the note keeper of your choice.

## Project structure

Project consists of 3 parts: 
* `pcb` - everything related to the schematics/PCB/boards' production. Currently v1.0, 1.1 and 1.2 exist, only v1.2 is supported in the software.
* `firmware` - software components for NRF52 chip. 
* `case` - STL files for 3D-printing the case for the Dictofun device.
* `doc` - folder with documentation used in the process of the development (datasheets, and potentially later schematic files).

`iOS` and `Android` counterparts are placed in a separate repository.

## Schematics

Relevant schematics for device is located here: `out/v1.2/schematic/nRF52832_qfaa.pdf`

## Build firmware

TBD.

Before building the firmware, SDK should be downloaded. To do that, go to `firmware/sdk` folder and execute script
`download_nordic_sdk.sh`. This command downloads and extracts the SDK files into the folder locally.

After the download one can open a corresponding Segger SES Project with SES. Later it should be possible to build with CMake. 

## Prerequisites

* install `gcc-arm-none-eabi-10.3-2021.10` to `/usr/share/gcc-arm-none-eabi-10.3-2021.10`.
* install `cmake` 3.22 or newer
* install `make`
* run script `firmware/sdk/download_nordic_sdk.sh`