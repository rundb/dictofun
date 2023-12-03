# Dictofun flashing harness

Special harness PCB has been developed to ease development of Dictofun. It is basically a bed of nails, where nails are connected
to pins of `Raspberry Pi Pico` evaluation board. It allows to combine flashing device, USB-UART bridge and button control into one 
single device and thus use only one USB connection to do all functions needed during the development.

In order to use this harness, following steps should be performed:
- flash Picoprobe firmware into Raspberry Pi Pico
- build OpenOCD with support of Picoprobe
- set up Picoprobe OpenOCD rules

After that flashing becomes possible.

## Picoprobe firmware

Picoprobe firmware implements a composite USB device with 2 devices: one used by OpenOCD for flashing and one - USB-UART bridge.

Here is the list of steps to perform (particular revision of repo is used in order to guarantee reproducibility):

```
# Assumption: we are in our working folder
# export DICTOFUN_PATH=/home/user/dictofun
git clone https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk
git checkout 6a7db34ff63345a7badec79ebea3aaef1712f374
git submodule update --init --recursive
cd ..
git clone https://github.com/raspberrypi/picoprobe.git
cd picoprobe
git checkout 7de418cce3dae75ad854029b14e8869955f0afaa
git submodule update --init --recursive
git apply $DICTOFUN_PATH/doc/harness/dictofun_harness_config.patch
mkdir build && cd build
export PICO_SDK_PATH=../../pico-sdk
cmake ..
make -j
```

This chain of commands results in file `picoprobe.uf2` residing in the `build` folder. This file should be used to flash Raspberry Pi Pico on 
the harness. After flashing this file (instructions for that can be found in Chapter 3.2.4 of [Getting started with Raspberry Pi Pico](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf)) Raspberry Pico becomes a flashing device as well as a USB-UART bridge.

## OpenOCD

OpenOCD has to be built according to `Appendix A` in [Getting started with Raspberry Pi Pico](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf) document (here - Linux only)

```
cd <DEVELOPMENT/FOLDER>
sudo apt install automake autoconf build-essential texinfo libtool libftdi-dev libusb-1.0-0-dev
git clone https://github.com/raspberrypi/openocd.git --branch rp2040 --depth=1
cd openocd
./bootstrap
./configure
make -j
sudo make install
```

## udev rules

I had to install `udev` rules as well. It might eventually work without it, as I haven't checked this scenario, but I don't think it's a bad idea to install them:

```
# /etc/udev/rules.d/99-pico.rules

# Make an RP2040 in BOOTSEL mode writable by all users, so you can `picotool`
# without `sudo`. 
SUBSYSTEM=="usb", ATTRS{idVendor}=="2e8a", ATTRS{idProduct}=="0003", MODE="0666"

# Symlink an RP2040 running MicroPython from /dev/pico.
#
# Then you can `mpr connect $(realpath /dev/pico)`.
SUBSYSTEM=="tty", ATTRS{idVendor}=="2e8a", ATTRS{idProduct}=="0005", SYMLINK+="pico"


#picoprobe
SUBSYSTEM=="usb", ATTRS{idVendor}=="2e8a", MODE="0666"

#after adding this execute
# sudo udevadm control --reload-rules &&  sudo udevadm trigger 
# connect and disconnetc the USB device
#
```
I have connected and disconnected the device after that and also rebooted the computer. After that I was able to launch flashing scripts.

## Usage

Following script performs flashing of an appropriate HEX file (assuming that build has been done):

```
cd <PATH/TO/DICTOFUN/firmware/build>
openocd -f interface/cmsis-dap.cfg -f target/nrf52.cfg -c "program ./src/targets/dictofun/Dictofun.hex verify reset exit"
```

This command performs a reset:

```
openocd -f interface/cmsis-dap.cfg -f target/nrf52.cfg -c "init ; reset run; exit" 
```
## Flash mass erase command

```
openocd -f interface/cmsis-dap.cfg -f target/nrf52.cfg -c "init; reset halt; nrf5 mass_erase; reset run; exit"
```

## Flashing Softdevice from bootloader working folder

```
openocd -f interface/cmsis-dap.cfg -f target/nrf52.cfg -c "program ../../../../../sdk/nRF5_SDK_17.1.0_ddde560/components/softdevice/s132/hex/s132_nrf52_7.2.0_softdevice.hex verify reset exit"
```
## Flashing Bootloader

```
openocd -f interface/cmsis-dap.cfg -f target/nrf52.cfg -c "program _build/nrf52832_xxaa_s132.hex verify reset exit"
```
