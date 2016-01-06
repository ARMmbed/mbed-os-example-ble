# BLE Heart Rate Monitor

This application transmits a heart rate value using the [Bluetooth SIG Heart Rate Profile](https://developer.bluetooth.org/TechnologyOverview/Pages/HRP.aspx). The heart rate value is provided by the application itself, not by a sensor, so that you don't have to get a sensor just to run the example. 

Technical details are better presented [in the mbed Classic equivalent of this example](https://developer.mbed.org/teams/Bluetooth-Low-Energy/code/BLE_HeartRate/).

# Running the application

## Requirements

To see the heart rate information on your phone, download Panobike for [iOS](https://itunes.apple.com/gb/app/panobike/id567403997?mt=8) or [Android](https://play.google.com/store/apps/details?id=com.topeak.panobike&hl=en).

You could also use a generic BLE scanners:

- [nRF Master Control Panel](https://play.google.com/store/apps/details?id=no.nordicsemi.android.mcp) for Android.

- [LightBlue](https://itunes.apple.com/gb/app/lightblue-bluetooth-low-energy/id557428110?mt=8) for iPhone.

Hardware requirements are in the [main readme](https://github.com/ARMmbed/ble-examples/blob/master/README.md).

## Building instructions

Building instructions for all samples are in the [main readme](https://github.com/ARMmbed/ble-examples/blob/master/README.md).

## Checking for success

1. Build the application and install it on your board as explained in the building instructions.

1. Open Panobike or the BLE scanner on your phone.

1. Find your device.

1. You should see the heart rate value change every half second. It begins at 100, goes up to 175 (in steps of 1), resets to 100 and so on.
