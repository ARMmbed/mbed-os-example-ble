Button count over GAP

This application shows how to use GAP to transmit a simple value every time that value is updated:

1. The value is a count of how many times a button on the device was pressed (the code actually monitors the button's releases, not press downs). 

1. We transmit the value in the SERVICE_DATA field of the advertising payload.

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

1. The Service Data field for your device should show the button press count. The starting value is 0.

1. Press the button on the device.

1. The Service Data field value should change.
