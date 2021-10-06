# DictoFun

Small wearable voice recorder. NRF52832-based.

## Concept

Device was initiated after my frustration while using voice recorder for storing ideas. It was taking up to 7 seconds from taking the voice recorder of the pocket to start recording the data. 

Idea: develop a small wearable device with just one button. When button is pressed, the device immediately starts recording the audio data. After the button is released, the record is then in the background synchronized with the phone.

The wearable device'es duty is done at this point. Phone's task is to save the record, apply voice recognition service and synchronize this data with `your favorite note keeper`

## Schematics

Schematics for device is located here: `out/v1.0(20.08.2021)/schematic/nRF52832_qfaa.pdf`

## Software

Project software consists of 2 large parts: embedded firmware for NRF52832 chip and Android phone software.

