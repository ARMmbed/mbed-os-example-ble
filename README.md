![](./resources/official_armmbed_example_badge.png)
# BLE Examples
This repo contains a collection of BLE example applications based on
mbed OS and built with [mbed-cli](https://github.com/ARMmbed/mbed-cli).
Each example subdirectory contains a separate mbed-cli module meant for building an executable.

Please browse to subdirectories for descriptions of the examples and any additional instructions on running them.

Getting Started
===============


Pre-Requisites
--------------

To build these examples, you need to have a computer with software installed as described [here](https://os.mbed.com/docs/latest/tools/index.html).

In order to use BLE in mbed OS you need one of the following hardware combinations:

* A Nordic nRF52-based board such as [nrf52DK](https://os.mbed.com/platforms/Nordic-nRF52-DK/)
* A supported target, such as the [NUCLEO-F401RE](https://os.mbed.com/platforms/ST-Nucleo-F401RE/), with a BLE shield or an external BLE peripheral, such as an [ST shield](https://os.mbed.com/components/X-NUCLEO-IDB04A1/).
* A [DISCO_L475VG_IOT01A (ref B-L475E-IOT01A)](https://os.mbed.com/platforms/ST-Discovery-L475E-IOT01A/) board.
* A [DISCO_L562QE (ref STM32L562E-DK)](https://os.mbed.com/platforms/ST-Discovery-L562QE/) board.
* A [NUCLEO_WB55RG](https://os.mbed.com/platforms/ST-Nucleo-WB55RG/) board.
* An Embedded Planet [Agora](https://os.mbed.com/platforms/agora-dev/) board

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

* ST boards with embedded BlueNrg module:
	* DISCO_L475VG_IOT01A (ref B-L475E-IOT01A)
	* DISCO_L562QE (ref STM32L562E-DK)

* Board with wireless STM32WB microcontrollers:
  * NUCLEO_WB55RG

* Embedded Planet:
	* EP_AGORA

	<span> **Important:** if an ST shield is used with the K64F board, an hardware is patch required. Check out https://developer.mbed.org/teams/ST/code/X_NUCLEO_IDB0XA1/ for more information.</span>
	
The following board is currently not supported as it doesn't yet support the Cordio stack:
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
* `"target.components_add": ["BlueNRG_MS"]` Add the BlueNRG_MS component to the target.
* `"target.features_add": ["BLE"]` Add the BLE feature to the target.
* `"target.extra_labels_add": ["CORDIO"]`: Add the BLE implementation of the ST shield to the list of the application modules.

As an example, this is the JSON bit which has to be added in the `target_overrides` section of `mbed_app.json` for a `NUCLEO_F411RE` board.

```json
        "NUCLEO_F411RE": {
            "target.components_add": ["BlueNRG_MS"],
            "target.features_add": ["BLE"],
            "target.extra_labels_add": ["CORDIO"]
        },
```

<span> **Note:** You can get more information about the configuration system in the [documentation](https://os.mbed.com/docs/latest/reference/configuration.html)</span>

<span> **Important:** It is required to apply an hardware patch to the ST shield if it is used on a board with an Arduino connector. Check out https://developer.mbed.org/teams/ST/code/X_NUCLEO_IDB0XA1/ for more information.</span>


Building and flashing examples
---------------------------------

__To build an example:__

1. Clone the repository containing the collection of examples:

	```
	$ git clone https://github.com/ARMmbed/mbed-os-example-ble.git
	```


	**Tip:** If you don't have git installed, you can [download a zip file](https://github.com/ARMmbed/mbed-os-example-ble/archive/master.zip) of the repository.

1. Using a command-line tool, navigate to any of the example directories, like BLE_Button:

	```
	$ cd mbed-os-example-ble
	$ cd BLE_Advertising
	```

1. Update the source tree:

	```
	mbed deploy
	```

1. Run the build:

	```mbed compile -t <ARM | GCC_ARM> -m <YOUR_TARGET>```

__To run the application on your board:__

1. Connect your mbed board to your computer over USB. It appears as removable storage.

1. When you run the `mbed compile` command above, mbed cli creates a .bin or a .hex file (depending on your target) in ```BUILD/<target-name>/<toolchain>``` under the example's directory. Drag and drop the file to the removable storage.

Running the examples
---------------------------------

When example application is running information about activity is printed over the serial connection.
The default serial baudrate has been set to 115200 for these examples.
Please have a client open and connected to the board. You may use:

- [Tera Term](https://ttssh2.osdn.jp/index.html.en) for windows

- screen or minicom for Linux (example usage: `screen /dev/serial/<your board> 115200`)

To observe and/or interact with example applications please use any BLE scanner on a smartphone.
If you don't have a scanner on your phone, please install :

- [nRF Master Control Panel](https://play.google.com/store/apps/details?id=no.nordicsemi.android.mcp) for Android.

- [LightBlue](https://itunes.apple.com/gb/app/lightblue-bluetooth-low-energy/id557428110?mt=8) for iPhone.

Known issues
============

* [NUCLEO_F411RE]: Some BLE examples doesn't work with the X-NUCLEO BLE shield. See [#40](https://github.com/ARMmbed/mbed-os-example-ble/issues/40)
* [NRF5] Impossible to debug or flash the examples with IAR: See [#39](https://github.com/ARMmbed/mbed-os-example-ble/issues/39)
* [NUCLEO_WB55RG]: some examples are not working with default application described in examples readme. Better use [LightBlue](https://itunes.apple.com/gb/app/lightblue-bluetooth-low-energy/id557428110?mt=8) for iPhone or try out [ST BLE Profile](https://play.google.com/store/apps/details?id=com.stm.bluetoothlevalidation).

### License and contributions

The software is provided under Apache-2.0 license. Contributions to this project are accepted under the same license. Please see contributing.md for more info.

This project contains code from other projects. The original license text is included in those source files. They must comply with our license guide
