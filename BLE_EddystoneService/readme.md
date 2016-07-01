Eddystone beacons broadcast a small amount of information, like URLs, to nearby BLE devices.

The Eddystone Beacon sample application runs in two stages:

1. On startup, the Configuration Service (which allows [modification of the beacon](https://github.com/google/eddystone/blob/master/eddystone-url/docs/config-service-spec.md)) runs for a user-defined period (default - 30 seconds).

1. When the Configuration Service period ends, the Eddystone Service broadcasts advertisement packets.



# Running the application

## Requirements

You should install the *Physical Web* application on your phone:

- [Android version](https://play.google.com/store/apps/details?id=physical_web.org.physicalweb)

- [iOS version](https://itunes.apple.com/us/app/physical-web/id927653608?mt=8)


**Note:** It is also possible to use a regular scanner to interract with your Eddystone beacon but it requires
knowledge about BLE and Eddystone beacon specification out of the scope of this document.


Hardware requirements are in the [main readme](https://github.com/ARMmbed/mbed-os-example-ble/blob/master/README.md).

## Building instructions

Building instructions for all samples are in the [main readme](https://github.com/ARMmbed/mbed-os-example-ble/blob/master/README.md).

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

1. Open the *Physical Web* application on your phone. It will start to search for nearby beacons.

    ![](img/app_start.png)

    **figure 1** Start of the *Physical Web* application version 0.1.856 on Android

1. When the beacon starts up, the Configuration Service runs for 30 seconds.
During this time it is possible to change the URL advertised by the beacon.
It is also important to note that during these 30 seconds, your device will not advertise any URL.

    ![](img/open_configuration.png)

    **figure 2** How to open the beacon configuration view using the *Physical Web* application version 0.1.856 on Android


1. Edit the URL advertised by your beacon.

    ![](img/edit_url.png)

    **figure 3** How to edit the URL advertised by your beacon using the *Physical Web* application version 0.1.856 on Android


1. Save the URL which will be advertised by your beacon.

    ![](img/save_url.png)

    **figure 4** How to save your beacon configuration and start advertising URL using the *Physical Web* application version 0.1.856 on Android.


1. Find your device; it should advertise the URL you have set.

    ![](img/result.png)

    **figure 5** Display of URL advertised by your beacon using the *Physical Web* application version 0.1.856 on Android.


**Note:** You can use the [Eddystone Observer](https://github.com/ARMmbed/mbed-os-example-ble/tree/master/BLE_EddystoneObserver) sample instead of a phone application.
