# Thermometer

This example uses the [Health Thermometer Profile](https://developer.bluetooth.org/gatt/services/Pages/ServiceViewer.aspx?u=org.bluetooth.service.health_thermometer.xml) to send thermometer information:

1. Sensor location: thermometer placement on the body. The default value in this application is the ear (``LOCATION_EAR``). The [characteristic description](https://developer.bluetooth.org/gatt/characteristics/Pages/CharacteristicViewer.aspx?u=org.bluetooth.characteristic.temperature_type.xml) shows the other possible values.

1. Temperature: the initial temperature is 39.6, and it's incremented by 0.1 every half second. It resets to 39.6 when it reaches 43.0.

For more information see:

* [Health Thermometer Service](https://developer.bluetooth.org/gatt/services/Pages/ServiceViewer.aspx?u=org.bluetooth.service.health_thermometer.xml): GATT profile details.

* [Temperature Measurement](https://developer.bluetooth.org/gatt/characteristics/Pages/CharacteristicViewer.aspx?u=org.bluetooth.characteristic.temperature_measurement.xml): GATT characteristic details for temperature measurement.

* [Temperature Type](https://developer.bluetooth.org/gatt/characteristics/Pages/CharacteristicViewer.aspx?u=org.bluetooth.characteristic.temperature_type.xml): GATT characteristic details for temperature type (sensor location).

# Running the application

## Requirements

The sample application can be seen on any BLE scanner on a smartphone. If you don't have a scanner on your phone, please install :

- [nRF Master Control Panel](https://play.google.com/store/apps/details?id=no.nordicsemi.android.mcp) for Android.

- [LightBlue](https://itunes.apple.com/gb/app/lightblue-bluetooth-low-energy/id557428110?mt=8) for iPhone.

Hardware requirements are in the [main readme](https://github.com/ARMmbed/mbed-os-example-ble/blob/master/README.md).

## Building instructions

Building instructions for all mbed OS samples are in the [main readme](https://github.com/ARMmbed/mbed-os-example-ble/blob/master/README.md).

## Checking for success

**Note:** Screens captures depicted below show what is expected from this example if the scanner used is *nRF Master Control Panel* version 4.0.5. If you encounter any difficulties consider trying another scanner or another version of nRF Master Control Panel. Alternative scanners may require reference to their manuals.

1. Build the application and install it on your board as explained in the building instructions.

1. Open the BLE scanner on your phone.

1. Start a scan.

    ![](img/start_scan.png)

    **figure 1** How to start scan using nRF Master Control Panel 4.0.5

1. Find your device; it should be named *Therm*.

    ![](img/scan_results.png)

    **figure 2** Scan results using nRF Master Control Panel 4.0.5

1. Establish a connection with your device.

    ![](img/connection.png)

    **figure 3**  How to establish a connection using Master Control Panel 4.0.5


1. Discover the services and the characteristics on the device. The *Health Thermometer* service has the UUID `0x1809` and includes the *Temperature Measurement* characteristic which has the UUID `0x2A1C`.

    ![](img/discovery.png)

    **figure 4** Representation of the Thermometer service using Master Control Panel 4.0.5


1. Register for the notifications sent by the *Temperature Measurement* characteristic.

    ![](img/register_to_notifications.png)

    **figure 5** How to register to notifications using Master Control Panel 4.0.5


1. You should see the temperature value change every half second. It begins at 39.6, goes up to  43.0 (in steps of 0.1), resets to 39.6 and so on.

    ![](img/notifications.png)

    **figure 6** Notifications view using Master Control Panel 4.0.5


