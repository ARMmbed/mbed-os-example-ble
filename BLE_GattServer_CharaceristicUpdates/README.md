# BLE Gatt Server example

This application demonstrates detailed uses of the GattServer APIs.

It starts by advertising to its environment with the device name "GattServer". Once you connect to the device with
a BLE scanner on your phone, the scanner shows a service with three characteristics - each representing the hour,
minute and second of a clock.

To see the clock values updating subscribe to the service using the "Enable CCCDs" (or similar) option provided
by the scanner. Now the values get updated once a second.

# Running the application

## Requirements

Hardware requirements are in the [main readme](https://github.com/ARMmbed/mbed-os-example-ble/blob/master/README.md).

## Building instructions

Building instructions for all samples are in the [main readme](https://github.com/ARMmbed/mbed-os-example-ble/blob/master/README.md).

