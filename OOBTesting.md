# ble-examples
This repo contains a collection of standard BLE example applications based on
mbed OS, and built with [yotta](https://github.com/ARMmbed/yotta). Each demo (sub-folder) contains a separate yotta-module
meant for building an executable.

For Out-of-Box testing, please test the following applications:

* BLE_HeartRate
* BLE_EddystoneBeacon
* BLE_EddystoneObserver (if you've got two BLE devices)

Please also browse to sub-folders for specific documentation.

Getting Started
===============

Hardware
--------

In order to use BLE in mbed OS you need one of the following hardware combinations:

Either...

 * A device with a radio on board, such as a Nordic nRF51-based board

or

 * A supported target, such as the FRDM K64f, whith a shield or external BLE peripheral, such as an ST shield

The [`ble` yotta module](https://github.com/ARMmbed/ble) is responsible for
providing the BLE APIs on mbed OS. The `ble` module uses yotta targets and yotta
target dependencies in order to provide the appropriate implementation of the BLE API
for your chosen hardware combination.

A yotta `target` is a supported combination of hardware platform and toolchain.

As such, for any of the hardware combinations above, you will need to use or create a
yotta target that describes your configuration. The existing supported configurations
are descibed below.

Targets for BLE
---------------

The following targets have been tested to work with these examples:

Nordic (using the nrf51822-ble module):

* nrf51dk-armcc
* nrf51dk-gcc
* mkit-gcc
* mkit-armcc

ST (using the st-ble module):

* frdm-k64f-st-ble-gcc (a FRDM-k64f with an ST BLE shield)

* When adding a new BLE-capable target, you should test it with these examples (most
of which are already included in the test-suite anyway) and then submit a pull request
on this repository to add the target to this list*

Building and testing the examples
---------------------------------

If using Nordic, please note that some of the Nordic repositories are still
limited to the ARM-private yotta registry. These will be published to the
public registry very shortly. This would require you to switch to the private
registry.

After cloning this repository, switch to any of the demo subdirectories, say
BLE_HeartRate, and execute the following:

```Shell
yotta target <an_appropriate_target_from_the_list_above>
yotta install
yotta build
```

The resulting binaries would be under `build/<yotta_target_name>/source/`.

Exactly which binaries are generated is dependent upon the target that you have
chosen. For Nordic Semiconductor targets, the following files will be present:

 * `<module_name>-combined.hex` is the one which can be flashed to the target
 * `<module_name>` is an ELF binary containing useful symbols
 * `<module_name>.hex` contains only the application (not the SoftDevice binary) and can be used for Firmware-over-the-Air.

For other targets, typically those that use external hardware like a shield, only the
`<module_name>` or `<module_name>.hex` files will be required. Either can be flashed
to the board, depending on which tools are being used to transfer the binary.

At this point, you should have the following

* A board with the appropriate hardware to use BLE
* A binary, running on this board, that makes use of the BLE APIs

In order to verify that the example are working properly, you should follow the
README.md of the specific example, which will explain in more detail what the example
does and how to test it.
