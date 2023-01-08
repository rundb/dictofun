# LittleFS integration

This module contains 2 different integrations of LittleFS:

- one module to run LittleFS using an API that runs in GoogleTest natively on the host.
- one module that implements access to SPI flash memory.

The native UT build explicitly checks correct behavior of LittleFS for the use-case of SPI Flash memory.
In particular, it's goal is to guarantee absence of expensive erase operations being executed during a 
file write process.