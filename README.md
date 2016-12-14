# mbed-os-porting
Test repository for porting mbed-os to the ubirch#1

# Building

- clone mbed-os and switch to branch `target-ubirch` to get the specific ubirch #1 changes
- link mbed-os into this directory at the top-level
- run `mbed compile`

# Flashing

- run `./bin/flash.sh` to flash using NXP blhost tool
- alternatively, if you have SEGGER tools installed, run `./bin/flash.sh -j`
