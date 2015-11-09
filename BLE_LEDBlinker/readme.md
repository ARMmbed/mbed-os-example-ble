The idea of this demo is to show you how to use the GattClient APIs that are
used to program BLE client devices. Hence, the client LEDBlinker device will
scan for advertising connections. Then it connects to an LED peripheral and
attempts to write the LED characteristic to toggle the LED state.

*Before you try this demo, make sure that the `peedAddr` filter os updated based on
the peripheral's MAC address.*

What You’ll Need
================

To get this going, you’ll need:

- Two mbed boards e.g. NRF51-DK.

- Flash one of the boards with the LED demo found [here](https://developer.mbed.org/teams/Bluetooth-Low-Energy/code/BLE_LED/).
  *Note that this application has not been ported to mbed OS yet!*

Checking for Success
====================

After loading the BLE_LEDBlink in one of the boards and the LED demo in a
second device, you should be able to see one of the LEDs from the second board
blinking. This is because the BLE_LEDBlink demo connects to the second device
through BLE and constantly toggles the value of the LED characteristic.

You also should be able to trace the execution of the program using a terminal
program and see if the demo exhibits the correct behaviour. To observe the output
you will need a terminal program that will listen to the output through a serial
port. There are many such programs available online depending on your operating
system. For Windows you can use 'Tera Term', for Mac OS X a good option is
'CoolTerm' and for Linux you can use 'GNU Screen' through the commanline.

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
