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
1. Find your device, it should appear with the name `Button` in the scanner.
1. Establish a connection with the device.
1. Discover the services and the characteristics on the device. The *Button service* has the UUID `0xA000` and includes the *Button state characteristic* which has the UUID `0xA001`.
1. Register for the notifications sent by the button state characteristic then the scanner will automatically receive a notification containing the new state of the button every time the state of the button changes.
1. Pressing Button 1 on your board updates the state of the button and sends a notification to the scanner. The new state of the button characteristic value should be equal to 0x01.
1. Releasing Button 1 on your board updates the state of the button and sends a notification to the scanner. The new state of the button characteristic value should be equal to 0x00.
