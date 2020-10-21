# SM - example usage of the Security Manager to pair and encrypt

Demonstration of possible usage of the Security Manager. Security Manager deals with pairing, authentication and encryption.

The application demonstrates usage as a central and a peripheral.

It will start advertising to demonstrate usage as a peripheral first. Please scan and connect using your peer device. Upon connection grant pairing if prompted. Upon success the application will disconnect.

The application will switch to demonstrating the central role and will connect to the device that previously connected to it. Please have it ready and advertising. Observe the terminal to keep track of the sequence. 

# Running the application

## Requirements

The sample application can be seen on any BLE scanner on a smartphone. If you don't have a scanner on your phone, please install :

- [nRF Master Control Panel](https://play.google.com/store/apps/details?id=no.nordicsemi.android.mcp) for Android.
NOTE: This second part of the example may not work on most Android devices that use random addresses.

- [LightBlue](https://itunes.apple.com/gb/app/lightblue-bluetooth-low-energy/id557428110?mt=8) for iPhone.
NOTE: On iOS, if the existing bond info becomes invalid (e.g. the example is reflashed) it needs to be removed in system settings.

Information about activity is printed over the serial connection - please have a client open. You may use:

- [Tera Term](https://ttssh2.osdn.jp/index.html.en)

Hardware requirements are in the [main readme](https://github.com/ARMmbed/mbed-os-example-ble/blob/master/README.md).

## Building instructions

Building instructions for all samples are in the [main readme](https://github.com/ARMmbed/mbed-os-example-ble/blob/master/README.md).

Note: this example currently is currently not supported on ST BLUENRG targets.
