Eddystone beacons broadcast a small amount of information, like URLs, to nearby BLE devices. 

The Eddystone Beacon sample application runs in two stages:

1. On startup, the Configuration Service (which allows [modification of the beacon](https://github.com/google/eddystone/blob/master/eddystone-url/docs/config-service-spec.md)) runs for a user-defined period (default - 30 seconds).

1. When the Configuration Service period ends, the Eddystone Service broadcasts advertisement packets.



# Running the application

## Requirements

The sample application can be seen on any BLE scanner on a smartphone. If you don't have a scanner on your phone, please install :

- [nRF Master Control Panel](https://play.google.com/store/apps/details?id=no.nordicsemi.android.mcp) for Android.

- [LightBlue](https://itunes.apple.com/gb/app/lightblue-bluetooth-low-energy/id557428110?mt=8) for iPhone.

Hardware requirements are in the [main readme](https://github.com/ARMmbed/ble-examples/blob/master/README.md).

## Building instructions

Building instructions for all samples are in the [main readme](https://github.com/ARMmbed/ble-examples/blob/master/README.md).

### Working with nRF51-based 16K targets

Because of memory constraints, you can't use the SoftDevice 130 (S130) to build for nRF51-based 16K targets. If you are using these targets, then before building:

1. Open the ``config.json`` file in this sample.
1. Change ``soft device`` to ``S110``.
1. Save.

You can now build for nRF51-based 16K targets.

## Setting up the beacon

By default, the beacon directs to the url ``http://mbed.org``. You can change this to your own URL in two ways:

1. Manually edit the code in ``main.cpp`` in your copy of the sample.

1. Build and run the application's default code as explained in the building instructions. When the beacon starts up, the Configuration Service runs for 30 seconds (this is the default value; you can change it in ``main.cpp``). While the Configuration Service runs, you can use a BLE scanner on your phone to edit the values the service presents.

## Checking for success

1. Build the application and install it on your board as explained in the building instructions.

1. Open the BLE scanner on your phone.

1. Find your device.

1. Check that the URL is correct.

You can use the [Eddystone Observer](https://github.com/ARMmbed/ble-examples/tree/master/BLE_EddystoneObserver) sample instead of a phone application.
