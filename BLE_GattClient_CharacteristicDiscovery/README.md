# BLE Gatt Client example of characteristic and service discovery

This application demonstrates detailed use of the `GattClient` APIs. 

When the application is started it advertises itself to its environment with the device name "GattClient"
and also tries to connect to any device with the name "GattServer". Once connection has been achieved
it will start a discovery of the GATT server exposed by the peer.

After the discovery, this application reads the value of the characteristics discovered and subscribes
to the characteristics emitting notifications or indications. 

The device prints the value of any indication or notification received from the peer's GATT Server.

# Running the application

## Requirements

Hardware requirements are in the [main readme](https://github.com/ARMmbed/mbed-os-example-ble/blob/master/README.md).

## Building instructions

Building instructions for all samples are in the [main readme](https://github.com/ARMmbed/mbed-os-example-ble/blob/master/README.md).
