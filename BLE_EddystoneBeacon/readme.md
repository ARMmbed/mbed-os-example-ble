Eddystone-Beacons are handy when there is a need to advertise a small amount of
information (usually a URL) to any nearby device. This example sets up an
Eddystone-Beacon, so all you’ll need to do is tell the beacon what to broadcast.

Technical details are better presented [here](https://developer.mbed.org/teams/Bluetooth-Low-Energy/code/BLE_EddystoneBeacon_Service/).

What You’ll Need
================

To get this going, you’ll need:

- To see BLE devices and their advertisement or beacon information, get one of the following installed on your phone:

  - The `physical web` app. You can get that app for [iOS](https://itunes.apple.com/us/app/physical-web/id927653608?mt=8) and for [Android](https://play.google.com/store/apps/details?id=physical_web.org.physicalweb).

  - For Android, you can get [nRF Master Control Panel](https://play.google.com/store/apps/details?id=no.nordicsemi.android.mcp).

  - For iPhone, you can get [LightBlue](https://itunes.apple.com/gb/app/lightblue-bluetooth-low-energy/id557428110?mt=8).

- One of the BLE platforms listed in the README.md of this repository, for example a
  Nordic DK board.

Build Instructions
==================

After cloning the parent repository, switch to the subfolder BLE_EddystoneBeacon, and
execute the following:

```Shell
yotta target <an_appropriate_target_such_as_mkit-gcc>
yotta install
yotta build
```

Assuming that you're building for the nRF51 DK platform, available targets are
`nrf51dk-armcc` and `nrf51dk-gcc`. You can pick either.

The resulting binaries would be under `build/<yotta_target_name>/source/`.
Under that folder, the file called `ble-eddystonebeacon-combined.hex` is the one which
can be flashed to the target using mbed's DAP over USB; the file called `ble-eddystonebeacon`
is an ELF binary containing useful symbols; whereas `ble-eddystonebeacon.hex`
can be used for Firmware-over-the-Air.

If you're building for the `nrf51dk-armcc` target, copy `build/nrf51dk-armcc/source/ble-eddystonebeacon-combined.hex`
to your target hardware, and reset the device.

Checking for Success
====================

Your Eddystone beacon should be detectable by BLE scanners (e.g. a smartphone) and by the
Google Physical Web app.

