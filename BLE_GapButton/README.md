GapButton
============
Broadcasting a count of button clicks over GAP. This is the simplest way to transmit information over BLE: all listening devices in range will see the broadcast and be able to read the button clicks.

What You’ll Need
================
- You can use one of the generic apps to scan BLE peripherals.
  - For Android [nRF Master Control Panel](https://play.google.com/store/apps/details?id=no.nordicsemi.android.mcp&hl=en).
  - For iPhone [LightBlue](https://itunes.apple.com/gb/app/lightblue-bluetooth-low-energy/id557428110?mt=8).
- One of the BLE platforms listed in the [README.md](https://github.com/ARMmbed/ble-examples/tree/oob-oct15) of this repository, for example a Nordic DK board.

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

The resulting binaries will be under `build/<yotta_target_name>/source/`.

Under that folder, the file called `ble-gapbutton<-combined>.hex` is the one which can be flashed to the target using mbed's DAP over USB; the parent README or the documentation for your yotta target will explain how to choose between the available binaries and hex files.

If you're building for the `nrf51dk-armcc` target, copy `build/nrf51dk-armcc/source/ble-gapbutton-combined.hex` to your target hardware, and reset the device.

Checking for Success
====================

By default the BLE device is called GapButton, but you can change this in `source\main.cpp`.

Open the BLE monitoring app on your phone and find the GapButton device. The button click count should show up in the "Manufacturer Data" field. Press Button 1 on your board and the number should change on your phone.
