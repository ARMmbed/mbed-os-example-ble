# Thermometer

This example uses the [Health Thermometer Profile](https://developer.bluetooth.org/gatt/services/Pages/ServiceViewer.aspx?u=org.bluetooth.service.health_thermometer.xml) to send thermometer information:

1. Sensor location: thermometer placement on the body. The default value in this application is the ear (``LOCATION_EAR``). The [characteristic description](https://developer.bluetooth.org/gatt/characteristics/Pages/CharacteristicViewer.aspx?u=org.bluetooth.characteristic.temperature_type.xml) shows the other possible values.

1. Temperature: the initial temperature is 39.6, and it's incremented by 0.1 every half second.

For more information see:

* [Temperature Service](https://developer.bluetooth.org/gatt/profiles/Pages/ProfileViewer.aspx?u=org.bluetooth.profile.health_thermometer.xml): GATT profile details.

* [Temperature Measurement](https://developer.bluetooth.org/gatt/characteristics/Pages/CharacteristicViewer.aspx?u=org.bluetooth.characteristic.temperature_measurement.xml): GATT characteristic details for temperature measurement.

* [Temperature Type](https://developer.bluetooth.org/gatt/characteristics/Pages/CharacteristicViewer.aspx?u=org.bluetooth.characteristic.temperature_type.xml): GATT characteristic details for temperature type (sensor location).

# Running the application

## Requirements

The sample application can be seen on any BLE scanner on a smartphone. If you don't have a scanner on your phone, please install :

- [nRF Master Control Panel](https://play.google.com/store/apps/details?id=no.nordicsemi.android.mcp) for Android.

- [LightBlue](https://itunes.apple.com/gb/app/lightblue-bluetooth-low-energy/id557428110?mt=8) for iPhone.

Hardware requirements are in the [main readme](https://github.com/ARMmbed/ble-examples/blob/master/README.md).

## Building instructions

Building instructions for all mbed OS samples are in the [main readme](https://github.com/ARMmbed/ble-examples/blob/master/README.md).

## Checking for success

1. Build the application and install it on your board as explained in the building instructions.

1. Open the BLE scanner on your phone.

1. Find your device.

1. Check that the temperature increments are shown.
