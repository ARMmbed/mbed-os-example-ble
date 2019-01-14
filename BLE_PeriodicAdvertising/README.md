# Periodic advertising and scanning

Demo of the periodic advertising. This requires two devices to run. Both devices run the same program. They attempt to find find each other after which they adopt complementary roles. One sets up periodic advertising. The other attempts to scan and sync with the periodic advertising.

The role of the scanner device can also be performed by a BLE scanner on a smartphone. Connect to the advertiser. This will establish it as the advertiser. After you disconnect the device will begin periodic advertising.

# Running the application

## Requirements

Devices must support extended advertising and periodic advertising (Bluetooth version 5+).

The sample application can also be monitored by any BLE scanner on a smartphone.

If you don't have a scanner on your phone, please install:

- [nRF Master Control Panel](https://play.google.com/store/apps/details?id=no.nordicsemi.android.mcp) for Android.

- [LightBlue](https://itunes.apple.com/gb/app/lightblue-bluetooth-low-energy/id557428110?mt=8) for iPhone.

Information about activity is printed over the serial connection - please have a client open. You may use:

- [Tera Term](https://ttssh2.osdn.jp/index.html.en)

## Building instructions

Building instructions for all samples are in the [main readme](https://github.com/ARMmbed/mbed-os-example-ble/blob/master/README.md).
