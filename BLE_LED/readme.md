To help you create your own BLE services, we have created this service template.
The LED example demonstrates the use of a read-write characteristic to control a
LED through a phone app.

The template covers:

* Setting up advertising and connection states.
* Assigning UUIDs to the service and its characteristic.
* Creating an input characteristic: read-write, boolean. This characteristic offers control of the LED.
* Constructing a service class and adding it to the BLE stack.

# Running the application

## Requirements

The sample application can be seen on any BLE scanner on a smartphone. If you don't have a scanner on your phone, please install :

- [nRF Master Control Panel](https://play.google.com/store/apps/details?id=no.nordicsemi.android.mcp) for Android.

- [LightBlue](https://itunes.apple.com/gb/app/lightblue-bluetooth-low-energy/id557428110?mt=8) for iPhone.

Hardware requirements are in the [main readme](https://github.com/ARMmbed/ble-examples/blob/master/README.md).

*NOTE:* If you have more than a single mbed board (e.g. nrf51dk or mkit) you can
run the BLE_LED and BLE_LEDBlinker at the same time. For more information please
refer to the BLE_LEDBlinker demo.

## Building instructions

Building instructions for all samples are in the [main readme](https://github.com/ARMmbed/ble-examples/blob/master/README.md).

## Checking for success

1. Build the application and install it on your board as explained in the building instructions.
1. Open the BLE scanner on your phone.
1. Find your device; it should be named `LED`.
1. Find the LED service; its UUID is `0xA000`. The exact instructions to do this depend on the scanner you're using.
1. Find the LED characteristic; its UUID is `0xA001`. The exact instructions to do this depend on the scanner you're using.
1. The characteristic accept a 1-bytes value:
    * `0x00`: LED ON
    * `0x01`: LED OFF
1. Toggle the LED characteristic value and see the LED turn ON or turn OFF according to the value you set.

If you can see the characteristic, and the LED is turned on/off as you toggle its value, the application is working properly.
