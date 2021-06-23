![](./resources/official_armmbed_example_badge.png)
# BLE Examples

This repo contains a collection of BLE example applications based on Mbed OS. Each example subdirectory contains a separate Mbed OS project, along with a description of the example and additional instructions for running it.

You can build each project with all supported [Mbed OS build tools](https://os.mbed.com/docs/mbed-os/latest/tools/index.html). However, this file specifically refers to the command-line interface tools, [Arm Mbed CLI 1](https://github.com/ARMmbed/mbed-cli#installing-mbed-cli) and [Mbed CLI 2](https://github.com/ARMmbed/mbed-tools#installation).

The [BLE documentation](https://os.mbed.com/docs/latest/reference/bluetooth.html) describes the BLE APIs on Mbed OS.

## Mbed OS build tools

### Mbed CLI 2
Starting with version 6.5, Mbed OS uses Mbed CLI 2. It uses Ninja as a build system, and CMake to generate the build environment and manage the build process in a compiler-independent manner. If you are working with Mbed OS version prior to 6.5 then check the section [Mbed CLI 1](#mbed-cli-1).

[Install Mbed CLI 2](https://os.mbed.com/docs/mbed-os/latest/build-tools/install-or-upgrade.html).

### Mbed CLI 1
[Install Mbed CLI 1](https://os.mbed.com/docs/mbed-os/latest/quick-start/offline-with-mbed-cli.html).

## Pre-Requisites


In order to use BLE in Mbed OS you need one of the following hardware combinations:

* A supported target, such as the [NUCLEO-F401RE](https://os.mbed.com/platforms/ST-Nucleo-F401RE/), with a BLE shield or an external BLE peripheral, such as an [X-NUCLEO-BNRG2A1](https://os.mbed.com/components/X-NUCLEO-BNRG2A1/) or an [X-NUCLEO-IDB05A1](https://os.mbed.com/components/X-NUCLEO-IDB05A1/) ST BLE expansion board.
* A [DISCO_L475VG_IOT01A (ref B-L475E-IOT01A)](https://os.mbed.com/platforms/ST-Discovery-L475E-IOT01A/) board.
* A [DISCO_L562QE (ref STM32L562E-DK)](https://os.mbed.com/platforms/ST-Discovery-L562QE/) board.
* A [NUCLEO_WB55RG](https://os.mbed.com/platforms/ST-Nucleo-WB55RG/) board.
* A Nordic nRF52-based board such as [nRF52DK](https://os.mbed.com/platforms/Nordic-nRF52-DK/).
* An Embedded Planet [Agora](https://os.mbed.com/platforms/agora-dev/) board.

The [BLE documentation](https://os.mbed.com/docs/latest/reference/bluetooth.html) describes the BLE APIs on mbed OS.

### Targets for BLE

The following targets have been tested and work with these examples:

* Targets with an ST BLE expansion board plugged in:
    * NUCLEO_F401RE
    * NUCLEO_L476RG
    * NUCLEO_L446RE
    * K64F

* ST boards with embedded SPBTLE-RF module (BlueNRG-MS):
    * DISCO_L475VG_IOT01A (ref B-L475E-IOT01A)
    * DISCO_L562QE (ref STM32L562E-DK)

* Board with wireless STM32WB microcontrollers:
    * NUCLEO_WB55RG

* Nordic:
    * NRF52_DK
    * NRF52840_DK

* Embedded Planet:
    * EP_AGORA

**Important:** If an ST BLE expansion is used with the K64F board, a hardware patch is required. Check out [X-NUCLEO-BNRG2A1](https://github.com/ARMmbed/mbed-os/tree/master/connectivity/drivers/ble/FEATURE_BLE/COMPONENT_BlueNRG_2) or [X-NUCLEO-IDB05A1](https://os.mbed.com/components/X-NUCLEO-IDB05A1/) for more information.

The following board is currently not supported as it doesn't yet support the Cordio stack:
    * NRF51_DK

### Using ST BLE expansion board on other targets

It is possible to use the ST BLE expansion on boards not directly supported by these examples as long as the board has an Arduino UNO R3 connector.

To make the board compatible with the ST BLE expansion three things are required:
* Add the BLE feature to your target.
* Add the BLE implementation for the ST BLE expansion to the list of modules which have to be compiled.
* Indicate to the BLE implementation that your board uses an Arduino connector.

All these operations can be done in the file `mbed_app.json` present in every example.

In the section `target_overrides`, add a new object named after your target with the following three fields:
* `"target.components_add": ["BlueNRG_2"]` Add the BlueNRG-2 component to the target.
* `"target.features_add": ["BLE"]` Add the BLE feature to the target.
* `"target.extra_labels_add": ["CORDIO"]`: Add the BLE implementation of the ST BLE expansion to the list of the application modules.

Below is an example of the JSON to be added in the `target_overrides` section of `mbed_app.json`, with the `NUCLEO_F401RE` board.

```json
        "NUCLEO_F401RE": {
            "target.components_add": ["BlueNRG_2"],
            "target.features_add": ["BLE"],
            "target.extra_labels_add": ["CORDIO"]
        },
```

**Note:** Further information about the configuration system is available in the [documentation](https://os.mbed.com/docs/latest/reference/configuration.html).

**Important:** It is required to apply an hardware patch to the ST BLE expansion if it is used on a board with an Arduino connector. Check out [X-NUCLEO-BNRG2A1](https://github.com/ARMmbed/mbed-os/tree/master/connectivity/drivers/ble/FEATURE_BLE/COMPONENT_BlueNRG_2) or [X-NUCLEO-IDB05A1](https://os.mbed.com/components/X-NUCLEO-IDB05A1/) for more information.


## Building the examples

1. Clone the repository containing the collection of examples:

    ```bash
    $ git clone https://github.com/ARMmbed/mbed-os-example-ble.git
    ```


    **Tip:** If you don't have git installed, you can [download a zip file](https://github.com/ARMmbed/mbed-os-example-ble/archive/master.zip) of the repository.

1. Using a command-line tool, navigate to any of the example directories, like BLE_Advertising:

    ```bash
    $ cd mbed-os-example-ble
    $ cd BLE_Advertising
    ```

1. Update the source tree:

    * Mbed CLI 2

    ```bash
    $ mbed-tools deploy
    ```
    
    * Mbed CLI 1

    ```bash
    $ mbed deploy
    ```

1. Connect a USB cable between the USB port on the board and the host computer.

1. Run the following command: this will build the example project, program the microcontroller flash memory, and then
open a serial terminal to the device.

    * Mbed CLI 2

    ```bash
    $ mbed-tools compile -m <TARGET> -t <TOOLCHAIN> --flash --sterm --baudrate 115200
    ```

    * Mbed CLI 1

    ```bash
    $ mbed compile -m <TARGET> -t <TOOLCHAIN> --flash --sterm --baudrate 115200
    ```

Your PC may take a few minutes to compile your code.

The binary will be located in the following directory:
* **Mbed CLI 2** - `./cmake_build/<TARGET>/<PROFILE>/<TOOLCHAIN>/`
* **Mbed CLI 1** - `./BUILD/<TARGET>/<TOOLCHAIN>/`

You can manually copy the binary to the target, which gets mounted on the host computer through USB, rather than using the `--flash` option.

You can also open a serial terminal separately, as explained below, rather than using the `--sterm` and `--baudrate` options.

## Running the examples

When example application is running, information about activity is printed over the serial connection.
The default serial baudrate has been set to 115200 for these examples.

If not using the `--sterm` and `--baudrate` options when flashing, have a client 
open and connected to the board. You may use:

- Mbed CLI 2 
    ```bash
    $ mbed-tools sterm -b 115200
    ```

- Mbed CLI 1
    ```bash
    $ mbed sterm -b 115200
    ```

- [Tera Term](https://ttssh2.osdn.jp/index.html.en) for Windows

- screen or minicom for Linux
    ```bash
    screen /dev/serial/<your board> 115200
    ```

To observe and/or interact with example applications please use any BLE scanner on a smartphone.
If you don't have a scanner on your phone, please install:

- [nRF Connect for Mobile](https://play.google.com/store/apps/details?id=no.nordicsemi.android.mcp) for Android and iOS.

- [ST BLE Profile](https://play.google.com/store/apps/details?id=com.stm.bluetoothlevalidation) for Android.


## Using bare metal profile

MBED BLE can support bare metal profile: https://os.mbed.com/docs/mbed-os/v6.10/bare-metal/using-the-bare-metal-profile.html


Here is an example with NUCLEO_WB55RG, update your local mbed_app.json:
```
{
    "requires": ["bare-metal", "events", "cordio-stm32wb"],
```

## How to reduce application size

Here are few tips to reduce further application size (this could be in addition of baremetal)

Update in mbed_app.json:

```
{
    "target_overrides": {
        "*": {
            "target.c_lib": "small",
            "target.printf_lib": "minimal-printf",
            "platform.minimal-printf-enable-floating-point": false,
            "platform.stdio-minimal-console-only": true,
...
```


## Troubleshooting

If you encounter problems with running the example, first try to update to the `development` branch of the example and
see if the problem persists. Make sure to run `mbed update` after you checkout the `development` branch to update the
libraries to the versions in that branch.

If the problem persists, try turning on traces in the example. This is done by changing the config in `mbed_app.json`:

```
		"mbed-trace.enable": true,
		"mbed-trace.max-level": "TRACE_LEVEL_DEBUG",
		"cordio.trace-hci-packets": true,
		"cordio.trace-cordio-wsf-traces": true,
		"ble.trace-human-readable-enums": true
```

Compile with `--profile debug` and run with the serial connected to your PC.

This will enable all the traces in BLE. If the number of traces is too big for the serial to handle or the image
doesn't fit try turning off all except the first one (`mbed-trace.enable`) and/or lowering the `max-level` to
`"TRACE_LEVEL_WARNING"`.

Save the output of the serial to a file. Please open an issue in this repo, describe the problem and attach the file
containing the trace output.

## License and contributions

The software is provided under Apache-2.0 license. Contributions to this project are accepted under the same license. Please see [CONTRIBUTING.md](./CONTRIBUTING.md) for more info.

## Branches

`Master` branch is for releases only. Please target the `development` branch for all your PRs.
