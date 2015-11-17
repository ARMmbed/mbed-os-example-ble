This example creates a BLE beacon: a method of advertising a small amount of information to nearby devices. The information doesn't have to be human-readable; it can be in a format that only an application can use.

Beacons are very easy to set up: the code for all beacons is the same, and only the information you want to advertise - the beacon payload - needs to change. 

This example advertises a UUID, a major and minor number and the transmission strength. The major and minor numbers are an example of information that is not (normally) meaningful to humans, but that an application can use to identify the beacon and display related information. For example, if the major number is a store ID and the minor number is a location in that store, then a matching application can use these numbers to query a database and display location-specific information.

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
1. View the beacon's details; the exact steps depend on which scanner you're using.

**Tip:** If you are in an area with many BLE devices, it may be difficult to identify your beacon. The simplest solution is to turn your board off and on, initiate a new scan on your BLE scanner every time, and look for the beacon that appears only when your board is on. 

If you can see the beacon and all its information, the application worked properly. 
