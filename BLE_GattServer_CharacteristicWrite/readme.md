# Create a writable characteristic on GattServer

This example creates a writable characteristic and notifies the user of any writes by printing to the serial.
The example will advertise as "GattServer" and await connection.

This example can be used together with `BLE_GattClient_CharacteristicWrite` running on another board or with
a phone running a BLE scanner application. Either one can connect and write a new value to the writable
characteristic with UUID `0xA001`.

As the application runs it will update you about its progress over serial and then proceed to print the value
of the writable characteristic every time it's updated.

# Running the application

## Requirements

Hardware requirements are in the [main readme](https://github.com/ARMmbed/mbed-os-example-ble/blob/master/README.md).

## Building instructions

Building instructions for all samples are in the [main readme](https://github.com/ARMmbed/mbed-os-example-ble/blob/master/README.md).
