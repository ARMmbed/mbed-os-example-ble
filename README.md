# BLE Examples
This repo contains a collection of BLE example applications based on
mbed OS and built with [mbed-cli](https://github.com/ARMmbed/mbed-cli). Each example subdirectory contains a separate mbed-cli module meant for building an executable.

Please browse to subdirectories for specific documentation.

Getting Started
===============


Pre-Requisites
--------------


To build these examples, you need to have a computer with the following software installed:

* [CMake](http://www.cmake.org/download/).
* [mbed-cli](https://github.com/ARMmbed/mbed-cli). Please note that **mbed-cli has its own set of dependencies**, listed in the installation instructions.
* [Python](https://www.python.org/downloads/).
* [ARM GCC toolchain](https://launchpad.net/gcc-arm-embedded).
* A serial terminal emulator (e.g. screen, pySerial, cu).
* If the OS used is Windows, the serial driver of the board has to be correctly installed.
	* For boards with mbed interface firmware the installation instructions are located (here)[https://developer.mbed.org/handbook/Windows-serial-configuration]
	* For nrf51-based board with a J-Link interface  please install the J-Link *software and documentation pack* available (here)[https://www.segger.com/jlink-software.html]


In order to use BLE in mbed OS you need one of the following hardware combinations:

* A Nordic nRF51-based board such as [nrf51dk](https://www.nordicsemi.com/eng/Products/nRF51-DK) or [mkit](https://www.nordicsemi.com/eng/Products/Bluetooth-R-low-energy/nRF51822-mKIT).
* A supported target, such as the [NUCLEO-F411RE](http://www.st.com/web/catalog/tools/FM116/SC959/SS1532/LN1847/PF260320), with a BLE shield or an external BLE peripheral, such as an [ST shield](http://www.st.com/web/catalog/tools/FM116/SC1075/PF260517).


The [`ble`module](https://github.com/ARMmbed/mbed-os/tree/master/bluetooth/ble) provides the BLE APIs on mbed OS.

Targets for BLE
---------------

The following targets have been tested and work with these examples:

* Nordic:
	* NRF51_DK
	* NRF52_DK

* Boards with an ST shield plugged in:
	* K64F
	* NUCLEO_F401RE

Building and testing the examples
---------------------------------

__To build an example:__

1. Clone the repository containing the collection of examples:

	```
	$ git clone https://github.com/ARMmbed/mbed-os-example-ble.git
	```


	**Tip:** If you don't have GitHub installed, you can [download a zip file](https://github.com/ARMmbed/mbed-os-example-ble/archive/master.zip) of the repository.

1. Using a command-line tool, navigate to any of the example directories, like BLE_Beacon:

	```
	$ cd ble-examples
	$ cd BLE_Beacon
	```

1. Update the source tree:

	```
	mbed update
	```

1. Run the build:

	```mbed compile -t <ARM | GCC_ARM> -m <YOUR_TARGET>```

__To run the application on your board:__

1. Connect your mbed board to your computer over USB. It appears as removable storage.

1. When you run the ``mbed compile`` command, as you did above, mbed cli creates a BIN or an HEX file in a ```.build/<target-name>/<toolchain>``` directory under the example's directory. Drag and drop the file to the removable storage.


Exactly which executables are generated depends on the target that you have
chosen. For Nordic Semiconductor targets, the following .hex files will be present:

 * `<module_name>.hex` is the one which can be flashed to the target.
 * `<module_name>.elf` is an ELF binary containing symbols (useful for debugging).



Creating or porting your own BLE applications in mbed OS
======================================================

If you're interested in creating BLE applications for mbed OS, or porting existing applications from mbed Classic to mbed OS, please see our [Introduction to mbed BLE](https://docs.mbed.com/docs/ble-intros/en/latest/mbed_OS/mbed_OS_BLE_Apps/).
