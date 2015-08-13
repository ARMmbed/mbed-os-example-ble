The heart rate example exports a virtual data-stream of heart-rate values.
This represents the use-case of a typical sensor driven app. In addition, this
application is also compatible with the [Bluetooth SIG heart rate profile](https://developer.bluetooth.org/TechnologyOverview/Pages/HRP.aspx).
That means that if you want to get a heart rate monitor’s input to your phone,
you don’t have to write your own code.

Technical details are better presented [here](https://developer.mbed.org/teams/Bluetooth-Low-Energy/code/BLE_HeartRate/),
which happens to be the mbed-classic equivalent of this example.

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

The resulting binaries would be under `build/<yotta_target_name>/source/`.
Under that folder, the file called `ble-heartrate-combined.hex` is the one which
can be flashed to the target using mbed's DAP over USB; the file called `ble-heartrate`
is an ELF binary containing useful symbols; whereas `ble-heartrate.hex`
can be used for Firmware-over-the-Air.

If you're building for the `nrf51dk-armcc` target, copy `build/nrf51dk-armcc/source/ble-heartrate-combined.hex`
to your target hardware, and reset the device. You should have an active
heart-rate detectable by heart-rate apps on smartphones.
