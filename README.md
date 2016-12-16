# mbed-os-porting
Test repository for porting mbed-os to the ubirch#1

# Building

- clone mbed-os and switch to branch `target-ubirch` to get the specific ubirch #1 changes
- link mbed-os into this directory at the top-level
	- `ln -sf <MBED-OS_DIR_PATH>/mbed-os .`
- add target `mbed target UBIRCH1`
- add toolchain `mbed toolchain GCC_ARM`
- run `mbed compile`

# Flashing

- run `./bin/flash.sh` to flash using NXP blhost tool
- alternatively, if you have SEGGER tools installed, run `./bin/flash.sh -j`

# Importing BME280 Library to the project
- In the project directory run
      `mbed add http://mbed.org/users/MACRUM/code/BME280/`
- run `mbed compile` 