Beacons are handy when there is a need to advertise a small amount of
information (usually a URL) to any nearby device. They’re really easy to set
up: the code is fully available on the mbed website, so all you’ll need to do
is tell the beacon what to broadcast.

Technical details are better presented [here](https://developer.mbed.org/teams/Bluetooth-Low-Energy/code/BLE_iBeacon/),
which happens to be the mbed-classic equivalent of this example.

What You’ll Need
================

To get this going, you’ll need:

* One of the generic apps to scan BLE peripherals.
** For Android, you can get [nRF Master Control Panel](https://play.google.com/store/apps/detailsid=no.nordicsemi.android.mcp&hl=en).
** For iPhone, you can get [LightBlue](https://itunes.apple.com/gb/app/lightblue-bluetooth-low-energy/id557428110?mt=8).

* An nRF51 DK board.

Build Instructions
==================

After cloning the parent repository, switch to the subfolder BLE_Beacon, and
execute the following:

```Shell
yotta target <an_appropriate_target_such_as_mkit-gcc>
yotta install
yotta build
```

Assuming that you're building for the nRF51 DK platform, available targets are
`nrf51dk-armcc` and `nrf51dk-gcc`. You can pick either.

The resulting binaries would be under `build/<yotta_target_name>/source/`.
Under that folder, the file called `ble-beacon-combined.hex` is the one which
can be flashed to the target using mbed's DAP over USB; the file called `ble-beacon`
is an ELF binary containing useful symbols; whereas `ble-beacon.hex`
can be used for Firmware-over-the-Air.

If you're building for the `nrf51dk-armcc` target, copy `build/nrf51dk-armcc/source/ble-beacon-combined.hex`
to your target hardware, and reset the device. You should have an active
beacon detectable by BLE scanners (e.g. a smartphone).
