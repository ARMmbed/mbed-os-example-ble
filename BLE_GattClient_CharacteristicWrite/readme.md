# Write characteristic with GattClient

This example demonstrates using the `GattClient` API to write to remote `GattServer`.

This example works best with the `BLE_GattServer_CharacteristicWrite` examples which provides the characteristic to write to.
Alternatively you can create your own GATT Server with a writable characteristic with UUID `0xA001` of size 1 byte.

This demo will run a GattClient and attempt to find a device with a name "GattServer". It will also accept a connection,
advertising as "GattClient".

After connection completes it will attempt to discover all services and characteristics, looking for the writable
characteristic. If it finds it, it will read and write the characteristic every 5 seconds, incrementing the byte value
every time.

Both applications should print the changing value in sync.

# Running the application

## Requirements

Hardware requirements are in the [main readme](https://github.com/ARMmbed/mbed-os-example-ble/blob/master/README.md).

## Building instructions

Building instructions for all mbed OS samples are in the [main readme](https://github.com/ARMmbed/mbed-os-example-ble/blob/master/README.md).
