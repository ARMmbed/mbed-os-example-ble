# BLE LED Blinker

This example demonstrates using the ``GattClient`` API to control BLE client devices.

The example uses two applications running on two different devices:

1. The first device - the central - runs the application ``BLE_LEDBlinker`` from this repository. This application sends an on/off toggle over BLE.

1. The second device - the peripheral - runs the application [``BLE_LED`` from our developer site](https://developer.mbed.org/teams/Bluetooth-Low-Energy/code/BLE_LED/) to respond to the toggle. 

	The toggle simply turns the LED on the peripheral device on and off.

# Running the application

## Requirements
 
Hardware requirements are in the [main readme](https://github.com/ARMmbed/ble-examples/blob/master/README.md).

This example works best if you have two devices, but [you can use a terminal program](#If-you-only-have-one-device) if you only have one device.

## Building instructions

You will need to build both applications and flash each one to a different board.

Please note:

1. The application ``BLE_LEDBlinker`` in this repository checks a device-specific parameter: ``peerAddr``. Before you build the application, you need to give this parameter the MAC address of your *peripheral* device (the device running the ``BLE_LED`` application).

	Building instructions for all mbed OS samples are in the [main readme](https://github.com/ARMmbed/ble-examples/blob/master/README.md).

	**Tip:** You may notice that the application also checks the LED characteristic's UUID; you don't need to change this parameter's value, because it already matches the UUID provided by the second application, ``BLE_LED``.

1.  The application ``BLE_LED``, [available on our developer site](https://developer.mbed.org/teams/Bluetooth-Low-Energy/code/BLE_LED/), is not an mbed OS application. You need to use [the online IDE](https://developer.mbed.org/compiler/#import:/teams/Bluetooth-Low-Energy/code/BLE_LED/;platform:) to build it.

## Checking for success

1. Build both applications and install one on each device, as explained in the building instructions.

1. The device running ``BLE_LED`` should blink its LED.

1. You can change the blinking speed by changing the callback period in ``BLE_LEDBlinker``:

	```
	minar::Scheduler::postCallback(periodicCallback).period(minar::milliseconds(500));
	```

	Rebuild the application and flash it to the device. The second device's LED should update its blink speed.

## If you have only one device

You can run ``BLE_LEDBlinker`` and see that it works properly by monitoring its serial output. 

You need a terminal program to listen to the output through a serial port. You can download one, for example:

* Tera Term for Windows.
* CoolTerm for Mac OS X.
* GNU Screen for Linux.

To see the application's output: 

1. Check which serial port your device is connected to.
1. Check the baud rate of your device.
1. Run a terminal program with the correct serial port and baud rate. For example, to use GNU Screen, run: ``screen /dev/tty.usbmodem1412 9600``.
1. The application should start printing the toggle's value to the terminal. 

