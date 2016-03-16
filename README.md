# BLE Examples
This repo contains a collection of BLE example applications based on
mbed OS and built with [yotta](https://github.com/ARMmbed/yotta). Each example subdirectory contains a separate yotta module meant for building an executable.

Please browse to subdirectories for specific documentation.

Getting Started
===============


Pre-Requisites
--------------


To build these examples, you need to have a computer with the following software installed:

* [CMake](http://www.cmake.org/download/).
* [yotta](https://github.com/ARMmbed/yotta). Please note that **yotta has its own set of dependencies**, listed in the [installation instructions](http://armmbed.github.io/yotta/#installing-on-windows).
* [Python](https://www.python.org/downloads/).
* [ARM GCC toolchain](https://launchpad.net/gcc-arm-embedded).
* A serial terminal emulator (e.g. screen, pySerial, cu).
* If the OS used is Windows, the serial driver of the board has to be correctly installed.
	* For boards with mbed interface firmware the installation instructions are located (here)[https://developer.mbed.org/handbook/Windows-serial-configuration]
	* For nrf51-based board with a J-Link interface  please install the J-Link *software and documentation pack* available (here)[https://www.segger.com/jlink-software.html]


In order to use BLE in mbed OS you need one of the following hardware combinations:

* A Nordic nRF51-based board such as [nrf51dk](https://www.nordicsemi.com/eng/Products/nRF51-DK) or [mkit](https://www.nordicsemi.com/eng/Products/Bluetooth-R-low-energy/nRF51822-mKIT).
* A supported target, such as the [NUCLEO-F411RE](http://www.st.com/web/catalog/tools/FM116/SC959/SS1532/LN1847/PF260320), with a BLE shield or an external BLE peripheral, such as an [ST shield](http://www.st.com/web/catalog/tools/FM116/SC1075/PF260517).


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

* st-nucleo-f401re-st-ble-gcc (a NUCLEO-F411RE board with an ST BLE shield)

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
