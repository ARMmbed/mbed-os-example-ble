# BLE Examples
This repo contains a collection of BLE example applications based on
mbed OS and built with [yotta](https://github.com/ARMmbed/yotta). Each example subdirectory contains a separate yotta module meant for building an executable.

Please browse to subdirectories for specific documentation.

Getting Started
===============

Hardware
--------

In order to use BLE in mbed OS you need one of the following hardware combinations:



 * A device with a radio on board, such as a Nordic nRF51-based board.

 * A supported target, such as the FRDM K64F, with a shield or external BLE peripheral, such as an ST shield.

The [`ble` yotta module](https://github.com/ARMmbed/ble) provides the BLE APIs on mbed OS. The `ble` module uses yotta targets and yotta
target dependencies to provide the appropriate implementation of the BLE API
for your chosen hardware combination.

A yotta `target` is a supported combination of hardware board and toolchain. This means that, for any of the hardware combinations above, you will need to use or create a
yotta target that describes your configuration. The existing supported configurations
are described below.

Targets for BLE
---------------

The following targets have been tested and work with these examples:

Nordic (using the nrf51822-ble module):

* nrf51dk-armcc
* nrf51dk-gcc
* mkit-gcc
* mkit-armcc

ST (using the st-ble module):

* frdm-k64f-st-ble-gcc (an FRDM-k64f with an ST BLE shield)

Building and testing the examples
---------------------------------

__To build an example:__

1. Clone the repository containing the collection of examples:

	```
	$ git clone https://github.com/ARMmbed/ble-examples.git
	```


	**Tip:** If you don't have GitHub installed, you can [download a zip file](https://github.com/ARMmbed/ble-examples/archive/master.zip) of the repository.

1. Using a command-line tool, navigate to any of the example directories, like BLE_Beacon:

	```
	$ cd ble-examples
	$ cd BLE_Beacon
	```

1. Set a yotta target. For example, if you have and Nordic nRF51 and the GCC toolchain:

	```
	yotta target nrf51dk-gcc
	```



1. Run the build:

	```yotta build```

__To run the application on your board:__

1. Connect your mbed board to your computer over USB. It appears as removable storage.

1. When you run the ``yotta build`` command, as you did above, yotta creates a BIN or a combined HEX file in a ```build/<target-name>/source``` directory under the example's directory. Drag and drop the file to the removable storage.


Exactly which executables are generated depends on the target that you have
chosen. For Nordic Semiconductor targets, the following .hex files will be present:

 * `<module_name>-combined.hex` is the one which can be flashed to the target.
 * `<module_name>` is an ELF binary containing symbols (useful for debugging).
 * `<module_name>.hex` contains only the application (not the SoftDevice binary) and can be used for Firmware Over the Air.


Creating or porting your own BLE applications in mbed OS
======================================================

If you're interested in creating BLE applications for mbed OS, or porting existing applications from mbed Classic to mbed OS, please see our [Introduction to mbed BLE](https://docs.mbed.com/docs/ble-intros/en/latest/mbed_OS/mbed_OS_BLE_Apps/).
