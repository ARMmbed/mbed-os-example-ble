# Example using Security and Privacy

Demonstration of Security Manager features. It shows how pairing and encryption works and how to use private addresses
when advertising and connecting and how filtering ties in with these operations.

The application will start by repeatedly trying to connect to the same application running on another board.
It will do this by advertising and scanning for random intervals waiting until the difference in intervals
between the boards will make them meet when one is advertising and the other scanning.

Two devices will be operating using resolvable private addresses. The application will connect to the peer and pair. 

Subsequent connections will turn on filtering based on peers identity addresses.  

# Running the application

## Requirements

Application requires two devices. Each one should be loaded with the same example. You may try to emulate the second
device with your phone. The demo advertise with the name "SecurityDemo". You must advertise with the same name.

Hardware requirements are in the [main readme](https://github.com/ARMmbed/mbed-os-example-ble/blob/master/README.md).

## Building instructions

Building instructions for all samples are in the [main readme](https://github.com/ARMmbed/mbed-os-example-ble/blob/master/README.md).

Note: example currently doesn't use ST provided stack and instead uses a Cordio port for the ST.
