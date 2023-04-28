#!/bin/bash

wget https://nsscprodmedia.blob.core.windows.net/prod/software-and-other-downloads/desktop-software/nrf-command-line-tools/sw/versions-10-x-x/10-21-0/nrf-command-line-tools_10.21.0_arm64.deb

sudo apt install ./nrf-command-line-tools_10.21.0_arm64.deb

sudo apt install /opt/nrf-command-line-tools/share/JLink_Linux_V780c_arm64.deb --fix-broken

wget https://github.com/NordicSemiconductor/nrf-udev/releases/download/v1.0.1/nrf-udev_1.0.1-all.deb

sudo dpkg -i nrf-udev_1.0.1-all.deb

sudo usermod -a -G dialout "$(whoami)"

rm -f "nrf-command-line-tools_10.21.0_arm64.deb"
rm -f "nrf-udev_1.0.1-all.deb"

sudo apt install -y picocom

echo "A re-login is required after execution of the script"
