# Dictofun bootloader

Secure DFU bootloader has been taken from 17.0.2 SDK with minimal modifications. 

All licenses are preserved. I intent to avoid any changes in this code. 

## Release process

Since a secure bootloader is utilized for this application, it has to be encrypted using `nrfutil` tools. 
Installation and usage instruction for the tool itself can be found at `https://infocenter.nordicsemi.com/index.jsp?topic=%2Fug_nrfutil%2FUG%2Fnrfutil%2Fnrfutil_intro.html`.

1. If it's first-ever execution of update, a private key should be generated.
   - `cd` into `<repository_root>/keys`.
   - run `nrfutil keys generate private.pem`. Now `private.pem` contains your private key.
   - run `nrfutil keys display --key pk --format code private.pem > ../firmware/src/targets/bootloader/dfu_public_key.c 2>&1`.

2. Package `hex`-image of application into DFU `.zip` package:
   - `cp ../firmware/src/targets/pca10040/Output/Debug/Exe/ble_app_blinky_pca10040_s132.hex ./app.hex` (example is for `pca10040` target).
   - `nrfutil pkg generate --hw-version 52 --sd-req 0x0101 --application-version 1 --application ble_app_blinky_pca10040_s132.hex --key-file private.pem app_dfu_package.zip`. Here `0x0101` is the code of SD 7.2.0, `application-version` should be set accordingly. 

`keys/app_dfu_package.zip` is the DFU package that can be used for the update.

## Update process

We plan to integrate DFU into our Dictofun application. Currently it's unavailalbe. Update can be done using `nRF Connect` software for `PC` and `Android`.

### PC

On PC one can install `nRF Connect` (see `https://www.nordicsemi.com/Products/Development-tools/nRF-Connect-for-desktop/Download#infotabs`). For update you would need a `NRF52840`-dongle or any of `NRF52 DK` evalboards.

When `nRF Connect` is installed, open the software (I'm using `nRF Connect v3.7.0` for Ubuntu). Open `Bluetooth Low Energy` app. 

In the `Bluetooth Low Energy` app select your dongle in the left top corner (there is a drop down list with all available COM devices, dongle is one of them).
If your device doesn't fit for DFU, the app will suggest to update your DK/dongle to an appropriate firmware. 

After a successful connection of the DK/dongle press `Start scan` in the right pane of the screen. When your `dictofun` is discovered there, press `Connect`.
In the main pane your device should appear. There is a `DFU` icon at the top of the corresponding square. Press the icon, select the corresponding DFU `zip` package and perform the update of your `dictofun`. 
