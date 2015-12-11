BLE_Button is a BLE service template. It handles a read-only characteristic with a simple input (boolean values). The input's source is the button on the board itself - the characteristic's value changes when the button is pressed or released.

The template covers:

1. Setting up advertising and connection modes.

1. Creating an input characteristic: read-only, boolean, with notifications. 

1. Constructing a service class and adding it to the BLE stack.

1. Assigning UUIDs to the service and its characteristic.

1. Pushing notifications when the characteristic's value changes.

# Running the application

## Requirements

The sample application can be seen on any BLE scanner on a smartphone. If you don't have a scanner on your phone, please install :

- [nRF Master Control Panel](https://play.google.com/store/apps/details?id=no.nordicsemi.android.mcp) for Android.

- [LightBlue](https://itunes.apple.com/gb/app/lightblue-bluetooth-low-energy/id557428110?mt=8) for iPhone.

Hardware requirements are in the [main readme](https://github.com/ARMmbed/ble-examples/blob/master/README.md).

## Building instructions

Building instructions for all samples are in the [main readme](https://github.com/ARMmbed/ble-examples/blob/master/README.md).

## Checking for success

1. Build the application and install it on your board as explained in the building instructions.
1. Open the BLE scanner on your phone.
1. Find your device.
1. Press the button on your board. 
1. The characteristic is automatically updated on every press and release, and the BLE scanner shows its value changes. 

