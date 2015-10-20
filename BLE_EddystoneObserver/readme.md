URI-Beacons are handy when there is a need to advertise a small amount of
information (usually a URL) to any nearby device. They’re really easy to set
up: the code is fully available on the mbed website, so all you’ll need to do
is tell the beacon what to broadcast. This example observes all URI-Beacon
broadcasts and prints the URIs in the advertising packets to the console.

Technical details are better presented [here](https://developer.mbed.org/teams/Bluetooth-Low-Energy/code/BLE_URIBeacon/).

What You’ll Need
================

To get this going, you’ll need:

- An nRF51 DK board.

- A device that acts as a URI-Beacon. You can run the beacon example found [here](https://developer.mbed.org/teams/Bluetooth-Low-Energy/code/BLE_URIBeacon/) if you have multiple nRF51 DK boards.

Build Instructions
==================

After cloning the parent repository, switch to the subfolder BLE_EddystoneObserver, and
execute the following:

```Shell
yotta target <an_appropriate_target_such_as_mkit-gcc>
yotta install
yotta build
```

Assuming that you're building for the nRF51 DK platform, available targets are
`nrf51dk-armcc` and `nrf51dk-gcc`. You can pick either.

The resulting binaries would be under `build/<yotta_target_name>/source/`.
Under that folder, the file called `ble-eddystoneobserver-combined.hex` is the one which
can be flashed to the target using mbed's DAP over USB; the file called `ble-eddystoneobserver`
is an ELF binary containing useful symbols; whereas `ble-eddystoneobserver.hex`
can be used for Firmware-over-the-Air.

If you're building for the `nrf51dk-armcc` target, copy `build/nrf51dk-armcc/source/ble-eddystoneobserver-combined.hex`
to your target hardware, and reset the device. You should have an active beacon
detectable by BLE scanners (e.g. a smartphone) for something to be printed in the console.
