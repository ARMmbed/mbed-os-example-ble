This example shows usage of the Security Manager as central and peripheral.
Security Manager deals with pairing, authentication and encryption.

# Running the application

As peripheral it will start advertising. Please scan and connect using your peer device.
Upon connection grant pairing if prompted. Upon success the application will disconnect.

The application will switch to demonstrating the central role and will connect to the device that previously connected to it.
Please have it ready and advertising. Observe the terminal to keep track of the sequence.

NOTE: This second part of the example when the device is a central may not work on most Android devices that use random addresses.
NOTE: On iOS, if the existing bond info becomes invalid (e.g. the example is reflashed) it needs to be removed in system settings.

## Requirements

Hardware requirements are in the [main readme](https://github.com/ARMmbed/mbed-os-example-ble/blob/master/README.md).

NOTE: This example currently is currently not supported on ST BLUENRG targets.

## Building instructions

Building instructions for all samples are in the [main readme](https://github.com/ARMmbed/mbed-os-example-ble/blob/master/README.md).
