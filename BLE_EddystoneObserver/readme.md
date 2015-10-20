Eddystone-Beacons are handy when there is a need to advertise a small amount of
information (usually a URL) to any nearby device. They’re really easy to set
up: the code is fully available on the mbed website, so all you’ll need to do
is tell the beacon what to broadcast. This example scans for Eddystone-Beacon
broadcasts and prints the URLs in the advertising packets to the console.

Technical details are better presented [here](https://developer.mbed.org/teams/Bluetooth-Low-Energy/code/BLE_URIBeacon/).

What You’ll Need
================

To get this going, you’ll need:

- An nRF51-DK board.

- A device that acts as an Eddystone-Beacon. You can run the BLE_EddystoneBeacon
  example in this repository if you have multiple nRF51-DK boards.

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

Checking Console Output
=======================

The Eddystone-Observer will scan for advertising packets and print to the serial
console a human-readable version of the URLs encoded within the Eddystone URL
frames. To observe the URLs you will need a terminal program that will listen to
the output through a serial port. There are many such programs available online
depending on your operating system. For Windows you can use 'Tera Term', for
Mac OS X a good option is 'CoolTerm' and for Linux you can use 'GNU Screen'
through the commanline.

Before trying to listen for output, make sure that your serial terminal is
listening through the correct serial port. Also, ensure that you set the
correct baud rate for your target platform. For instance, the NRF51-DK board
requires a baud rate of 9600; therefore, if I was trying to listen for output
using 'GNU Screen' the command to start the serial terminal will look like
this:

```Shell
screen /dev/tty.usbmodem1412 9600
```

Note that in my case `/dev/tty.usbmodem1412` is where the terminal program
will be listening.

