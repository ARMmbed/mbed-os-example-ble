Beacons are handy when there is a need to advertise a small amount of
information (usually a URL) to any nearby device. They’re really easy to set
up: the code is fully available on the mbed website, so all you’ll need to do
is tell the beacon what to broadcast.

Technical details are better presented [here](https://developer.mbed.org/teams/Bluetooth-Low-Energy/code/BLE_iBeacon/),
which happens to be the mbed-classic equivalent of this example.

What You’ll Need
================

To get this going, you’ll need:

- One of the generic apps to scan BLE peripherals.

  - For Android, you can get [nRF Master Control Panel](https://play.google.com/store/apps/detailsid=no.nordicsemi.android.mcp&hl=en).

  - For iPhone, you can get [LightBlue](https://itunes.apple.com/gb/app/lightblue-bluetooth-low-energy/id557428110?mt=8).

- One of the BLE platforms listed in the README.md of this repository, for example a
  Nordic DK board.

Build Instructions
==================

After cloning the parent repository, switch to the subfolder BLE_Beacon, and
execute the following:

```Shell
yotta target <an_appropriate_target_such_as_mkit-gcc>
yotta install
yotta build
```

The targets you can use are described in the main README.md for this repository.

The resulting binaries would be under `build/<yotta_target_name>/source/`.

Under that folder, the file called `ble-beacon<-combined>.hex` is the one which
can be flashed to the target using mbed's DAP over USB; the parent README or the
documentation for your yotta target will explain how to choose between the available
binaries and hex files.

If you're building for the `nrf51dk-armcc` target, copy
`build/nrf51dk-armcc/source/ble-beacon-combined.hex` to your target hardware, and
reset the device.

Checking for Success
====================

You will need to use another device to determine whether the beacon is running
properly. Using one of the BLE scanning apps above, search for nearby devices and you
should see an iBeacon device nearby.

If you are in an area with many BLE devices, try powering down your board and ensuring
that the beacon is not present when you rescan (some OSs cache the nearby beacons, so
be sure to do a new scan, rather than expecting the GUI to update to show the beacon
has gone).



