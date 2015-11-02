The heart rate example exports a virtual data-stream of heart-rate values.
This represents the use-case of a typical sensor driven app. In addition, this
application is also compatible with the [Bluetooth SIG heart rate profile](https://developer.bluetooth.org/TechnologyOverview/Pages/HRP.aspx).
That means that if you want to get a heart rate monitor’s input to your phone,
you don’t have to write your own code.

Technical details are better presented [here](https://developer.mbed.org/teams/Bluetooth-Low-Energy/code/BLE_HeartRate/),
which happens to be the mbed-classic equivalent of this example.

What You’ll Need
================

To get this going, you’ll need:

- To see the heart rate information on your phone, download Panobike for [iOS](https://itunes.apple.com/gb/app/panobike/id567403997?mt=8) or [Android](https://play.google.com/store/apps/details?id=com.topeak.panobike&hl=en).

- You could also use one of the generic apps to scan BLE peripherals.

  - For Android, you can get [nRF Master Control Panel](https://play.google.com/store/apps/details?id=no.nordicsemi.android.mcp).

  - For iPhone, you can get [LightBlue](https://itunes.apple.com/gb/app/lightblue-bluetooth-low-energy/id557428110?mt=8).

- One of the BLE platforms listed in the README.md of this repository, for example a
  Nordic DK board.

Build Instructions
==================

After cloning the parent repository, switch to the subfolder BLE_HeartRate, and
execute the following:

```Shell
yotta target <an_appropriate_target_such_as_mkit-gcc>
yotta install
yotta build
```

Assuming that you're building for the nRF51 DK platform, available targets are
`nrf51dk-armcc` and `nrf51dk-gcc`. You can pick either.

The other targets you can use are described in the main README.md for this repository.

The resulting binaries would be under `build/<yotta_target_name>/source/`.

Under that folder, the file called `ble-heartrate-combined.hex` is the one which
can be flashed to the target using mbed's DAP over USB; the parent README or the
documentation for your yotta target will explain how to choose between the available
binaries and hex files.

Checking for Success
====================

By default the heart rate monitor is called HRM, but you can change this on line 24 of
`source\main.cpp`.

Using one of the apps suggested above, connect to the device, and observe that the
heartrate is constantly increasing up to 175 and wrapping back to 100.
