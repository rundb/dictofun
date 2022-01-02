# DictoFun

Small wearable NRF52-based voice recorder. NRF52832-based.

![DictoFun](https://github.com/rundb/dictofun/blob/master/pcb/out/v1.1/dictofun_1.1_top.PNG?raw=true)

## Concept

Device was initiated after my frustration while using voice recorder for storing ideas. It was taking up to 7 seconds from taking the voice recorder of the pocket to start recording the data. 

Idea behind the device: press and hold the big button on the device and start speaking. The device should start recording _immediately_ (== under human reaction time, <=0.4 sec>), without the need for any other presses or for looking at the screen. When you're done - release the button. This should end the record. 

After finishing the record device should transfer the record to the phone. The record is then transferred to the voice recognition service. The recognized text should then be sent to the note keeper of your choice.

## Project structure

Project consists of 3 parts: 
* `pcb` - everything related to the schematics/PCB/boards' production. Currently v1.0 and 1.1 exist, only v1.1 is supported in the software.
* `firmware` - software components for NRF52 chip. 
* `android` - software that is running on the Android side. 

iOS might be also added later to this list.

## Schematics

Relevant schematics for device is located here: `out/v1.1/schematic/nRF52832_qfaa.pdf`

## Build firmware

Before building the firmware, SDK should be downloaded. To do that, go to `firmware/sdk` folder and execute script
`download_nordic_sdk.sh`. This command downloads and extracts the SDK files into the folder locally.

After the download one can open a corresponding Segger SES Project with SES. Later it should be possible to build with CMake. 

Android app can be built with a standard Android studio build process.
