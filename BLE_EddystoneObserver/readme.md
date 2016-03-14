The Eddystone Observer scans for Eddystone beacons that are running the [Eddystone Service example](https://github.com/ARMmbed/ble-examples/tree/master/BLE_EddystoneService) (see there for general information about Eddystone beacons). It reads the advertising packets broadcast by these beacons, and prints a human-readable version of the advertised URLs to the serial console.

# Running the application

## Requirements

General hardware information is in the [main readme](https://github.com/ARMmbed/ble-examples/blob/master/README.md).

This sample requires two devices - one to [broadcast the beacon](https://github.com/ARMmbed/ble-examples/tree/master/BLE_EddystoneService) and one to scan for the broadcast. If you have more devices, you can use them as extra beacons.

You need a terminal program to listen to the observer's output through a serial port. You can download one, for example:

* Tera Term for Windows.

* CoolTerm for Mac OS X.

* GNU Screen for Linux.

## Building instructions

Building instructions for all samples are in the [main readme](https://github.com/ARMmbed/ble-examples/blob/master/README.md).

1. Build and run the [Eddystone beacon](https://github.com/ARMmbed/ble-examples/tree/master/BLE_EddystoneService) on one or more other devices.

1. Build the Eddystone Observer application and install it on your board as explained in the building instructions. Leave the board connected to your computer.

## Checking console output

To see the application's output:

1. Check which serial port your Eddystone Observer is connected to.

1. Run a terminal program with the correct serial port and the baud rate set to 9600. For example, to use GNU Screen, run: ``screen /dev/tty.usbmodem1412 9600``.

1. The Eddystone Observer should start printing URLs of nearby Eddystone beacons to the terminal.
