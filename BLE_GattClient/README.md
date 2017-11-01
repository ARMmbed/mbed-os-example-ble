# BLE Gatt Client example

This application demonstrates detailed uses of the GattClient APIs. 

When the application is started it advertises itself to its environment with the 
device name `GattClient`. Once you have connected to the device with your mobile 
phone, the application starts a discovery of the GATT server exposed by your 
mobile phone. 

After the discovery, this application reads the value of the characteristics 
discovered and subscribes to the characteristics emitting notifications or 
indications. 

The device prints the value of any indication or notification received from the 
mobile phone.

# Running the application

## Requirements

You may use a generic BLE scanners:

- [nRF Master Control Panel](https://play.google.com/store/apps/details?id=no.nordicsemi.android.mcp) for Android.

- [LightBlue](https://itunes.apple.com/gb/app/lightblue-bluetooth-low-energy/id557428110?mt=8) for iPhone.

Hardware requirements are in the [main readme](https://github.com/ARMmbed/mbed-os-example-ble/blob/master/README.md).

## Building instructions

Building instructions for all samples are in the [main readme](https://github.com/ARMmbed/mbed-os-example-ble/blob/master/README.md).


