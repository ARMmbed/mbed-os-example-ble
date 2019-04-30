# BLE Examples
This repo contains a collection of BLE example applications based on
mbed OS and built with [mbed-cli](https://github.com/ARMmbed/mbed-cli). Each example subdirectory contains a separate mbed-cli module meant for building an executable.

Please browse to subdirectories for specific documentation.

Getting Started
===============


Pre-Requisites
--------------

To build these examples, you need to have a computer with software installed as described [here](https://os.mbed.com/docs/latest/tools/index.html).

In order to use BLE in mbed OS you need one of the following hardware combinations:

* A Nordic nRF52-based board such as [nrf52DK](https://os.mbed.com/platforms/Nordic-nRF52-DK/)
* A supported target, such as the [NUCLEO-F401RE](http://www.st.com/en/evaluation-tools/nucleo-f401re.html), with a BLE shield or an external BLE peripheral, such as an [ST shield](http://www.st.com/web/catalog/tools/FM116/SC1075/PF260517).
* A [DISCO_L475VG_IOT01A (ref B-L475E-IOT01A)](http://www.st.com/en/evaluation-tools/b-l475e-iot01a.html) board.

This target is currently not supported as the examples use Cordio link layer which doesn't have an implementation for it yet:
* A Nordic nRF51-based board such as [nrf51dk](https://www.nordicsemi.com/eng/Products/nRF51-DK) or [mkit](https://www.nordicsemi.com/eng/Products/Bluetooth-R-low-energy/nRF51822-mKIT).
You may still use the deprecated examples for this target. The deprecated examples are located in the "deprecated" folder.

The [BLE documentation](https://os.mbed.com/docs/latest/reference/bluetooth.html) describes the BLE APIs on mbed OS.

Targets for BLE
---------------

The following targets have been tested and work with these examples:

* Nordic:
	* NRF52_DK
	* NRF52840_DK

* Boards with an ST shield plugged in:
	* K64F
	* NUCLEO_F401RE

* STMicroelectronics:
	* DISCO_L475VG_IOT01A (ref B-L475E-IOT01A)
	* NUCLEO_WB55RG

	<span> **Important:** if an ST shield is used with the K64F board, an hardware is patch required. Check out https://developer.mbed.org/teams/ST/code/X_NUCLEO_IDB0XA1/ for more information.</span>
	
<span> Note: The Cordio Link layer is used for NRF52 platforms by default. To switch to using the Softdevice, remove these lines in mbed_app.json.
Bluetooth 5 features are only supported by the Cordio implementation, and some examples (such as BLE_GAP) use these. If you're using the Softdevice you can use the deprecated examples in this case.

For NUCLEO_WB55RG: the NUCLEO_WB55RG supports BLE CORDIO by default in mbed-os so there is no extra label in mbed_app.json.

For NRF52_DK:
```
    "target.extra_labels_add": ["CORDIO", "CORDIO_LL", "SOFTDEVICE_NONE", "NORDIC_CORDIO"],
    "target.extra_labels_remove": ["SOFTDEVICE_COMMON", "SOFTDEVICE_S132_FULL", "NORDIC_SOFTDEVICE"]
```

For NRF52840_DK:
```
    "target.extra_labels_add": ["CORDIO", "CORDIO_LL", "SOFTDEVICE_NONE", "NORDIC_CORDIO"],
    "target.extra_labels_remove": ["SOFTDEVICE_COMMON", "SOFTDEVICE_S140_FULL", "NORDIC_SOFTDEVICE"]
```
    
The following board is currently not supported by non-deprecated examples as it doesn't yet support the Cordio stack:
	* NRF51_DK

### Using ST Nucleo shield on other targets

It is possible to use the ST Nucleo shield on boards not directly supported by these examples as long as the board has an Arduino UNO R3 connector.

To makes the board compatible with the ST shield three things are required:
* Add the BLE feature to your target.
* Add the BLE implementation for the ST shield to the list of modules which have to be compiled.
* Indicate to the BLE implementation that your board use an Arduino connector.

All these operations can be done in the file `mbed_app.json` present in every example.

In the section `target_overrides` add a new object named after your target.
In this object two fields are required:
* `"target.features_add": ["BLE"]` Add the BLE feature to the target.
* `"target.extra_labels_add": ["CORDIO", "CORDIO_BLUENRG"]`: Add the BLE implementation of the ST shield to the list of the application modules.

As an example, this is the JSON bit which has to be added in the `target_overrides` section of `mbed_app.json` for a `NUCLEO_F411RE` board.

```json
        "NUCLEO_F411RE": {
            "target.features_add": ["BLE"],
            "target.extra_labels_add": ["CORDIO", "CORDIO_BLUENRG"]
        },
```

<span> **Note:** You can get more information about the configuration system in the [documentation](https://os.mbed.com/docs/latest/reference/configuration.html)</span>

<span> **Important:** It is required to apply an hardware patch to the ST shield if it is used on a board with an Arduino connector. Check out https://developer.mbed.org/teams/ST/code/X_NUCLEO_IDB0XA1/ for more information.</span>


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
	$ cd mbed-os-example-ble
	$ cd BLE_Beacon
	```

1. Update the source tree:

	```
	mbed deploy
	```

1. Run the build:

	```mbed compile -t <ARM | GCC_ARM> -m <YOUR_TARGET>```

__To run the application on your board:__

1. Connect your mbed board to your computer over USB. It appears as removable storage.

1. When you run the ``mbed compile`` command, as you did above, mbed cli creates a BIN or an HEX file in a ```BUILD/<target-name>/<toolchain>``` directory under the example's directory. Drag and drop the file to the removable storage.


Exactly which executables are generated depends on the target that you have
chosen. For Nordic Semiconductor targets, the following .hex files will be present:

 * `<module_name>.hex` is the one which can be flashed to the target.
 * `<module_name>.elf` is an ELF binary containing symbols (useful for debugging).

**Note:** Depending on the build process, the file which has to be flashed on a Nordic target can also be named `<module_name>-combined.hex`. If `<module_name>-combined.hex` and `<module_name>.hex` are present in the build directory, flash `<module_name>-combined.hex.

**Note:** On non Nordic targets, the file to flash can also be named `<module_name>.bin`. Refer to mbed-cli, mbed-os and your board vendor documentation for more informations.


Known issues
============

* [NUCLEO_F411RE]: Some BLE examples doesn't work with the X-NUCLEO BLE shield. See [#40](https://github.com/ARMmbed/mbed-os-example-ble/issues/40)
* [NRF5] Impossible to debug or flash the examples with IAR: See [#39](https://github.com/ARMmbed/mbed-os-example-ble/issues/39)
* [NUCLEO_WB55RG]: some examples are not working with default application described in examples readme. Better use [LightBlue](https://itunes.apple.com/gb/app/lightblue-bluetooth-low-energy/id557428110?mt=8) for iPhone or try out [ST BLE Profile](https://play.google.com/store/apps/details?id=com.stm.bluetoothlevalidation).

### License and contributions

The software is provided under Apache-2.0 license. Contributions to this project are accepted under the same license. Please see contributing.md for more info.

This project contains code from other projects. The original license text is included in those source files. They must comply with our license guide
