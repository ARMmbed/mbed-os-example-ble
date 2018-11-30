# BLE LED Blinker

This example demonstrates using the ``GattClient`` API to control BLE client devices.

The example uses two applications running on two different devices:

1. The first device - the central - runs the application ``BLE_LEDBlinker`` from this repository. This application sends an on/off toggle over BLE.

1. The second device - the peripheral - runs the application [``BLE_LED``](https://github.com/ARMmbed/mbed-os-example-ble/tree/master/BLE_LED) to respond to the toggle.

	The toggle simply turns the LED on the peripheral device on and off.

# Running the application

## Requirements

Hardware requirements are in the [main readme](https://github.com/ARMmbed/mbed-os-example-ble/blob/master/README.md).

This example requires *two* devices.

## Building instructions

You will need to build both applications and flash each one to a different board.

Please note: The application ``BLE_LEDBlinker`` in this repository initiate a connection to all ble devices which advertise "LED" as complete local name. By default, the application `BLE_LED` advertise "LED" as complete local name. If you change the local name advertised by the application `BLE_LED` you should reflect your change in this application by changing the value of the constant `PEER_NAME` in `main.cpp`.

**Tip:** You may notice that the application also checks the LED characteristic's UUID; you don't need to change this parameter's value, because it already matches the UUID provided by the second application, ``BLE_LED``.

Building instructions for all mbed OS samples are in the [main readme](https://github.com/ARMmbed/mbed-os-example-ble/blob/master/README.md).

## Checking for success

1. Build both applications and install one on each device, as explained in the building instructions.

1. The LED number two of the device running ``BLE_LED`` should blink.


## Monitoring the application through a serial port

You can run ``BLE_LEDBlinker`` and see that it works properly by monitoring its serial output.

You need a terminal program to listen to the output through a serial port. You can download one, for example:

* Tera Term for Windows.
* CoolTerm for Mac OS X.
* GNU Screen for Linux.

To see the application's output:

1. Check which serial port your device is connected to.
1. Run a terminal program with the correct serial port and set the baud rate to 9600. For example, to use GNU Screen, run: ``screen /dev/tty.usbmodem1412 9600``.
1. The application should start printing the toggle's value to the terminal.

**Note:** ``BLE_LEDBlinker`` will not run properly if the ``BLE_LED`` application is not running on a second device. The terminal will show a few print statements, but you will not be able to see the application in full operation.
