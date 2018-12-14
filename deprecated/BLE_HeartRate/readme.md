# BLE Heart Rate Monitor

This application transmits a heart rate value using the [Bluetooth SIG Heart Rate Profile](https://developer.bluetooth.org/TechnologyOverview/Pages/HRP.aspx). The heart rate value is provided by the application itself, not by a sensor, so that you don't have to get a sensor just to run the example.

Technical details are better presented [in the mbed Classic equivalent of this example](https://developer.mbed.org/teams/Bluetooth-Low-Energy/code/BLE_HeartRate/).

# Running the application

## Requirements

To see the heart rate information on your phone, use a BLE scanner:

- [nRF Master Control Panel](https://play.google.com/store/apps/details?id=no.nordicsemi.android.mcp) for Android.

- [LightBlue](https://itunes.apple.com/gb/app/lightblue-bluetooth-low-energy/id557428110?mt=8) for iPhone.

Hardware requirements are in the [main readme](https://github.com/ARMmbed/mbed-os-example-ble/blob/master/README.md).

## Building instructions

Building instructions for all samples are in the [main readme](https://github.com/ARMmbed/mbed-os-example-ble/blob/master/README.md).

## Checking for success

**Note:** Screens captures depicted below show what is expected from this example if the scanner used is *nRF Master Control Panel* version 4.0.5. If you encounter any difficulties consider trying another scanner or another version of nRF Master Control Panel. Alternative scanners may require reference to their manuals.

1. Build the application and install it on your board as explained in the building instructions.
1. Open the BLE scanner on your phone.
1. Start a scan.

    ![](img/start_scan.png)

    **figure 1** How to start scan using nRF Master Control Panel 4.0.5

1. Find your device; it should be named `HRM`.

    ![](img/scan_result.png)

    **figure 2** Scan results using nRF Master Control Panel 4.0.5

1. Establish a connection with your device.

    ![](img/connection.png)

    **figure 3**  How to establish a connection using Master Control Panel 4.0.5

1. Discover the services and the characteristics on the device. The *Heart Rate* service has the UUID `0x180D` and includes the *Heart Rate Measurement* characteristic which has the UUID `0x2A37`.

    ![](img/discovery.png)

    **figure 4** Representation of the Heart Rate service using Master Control Panel 4.0.5

1. Register for the notifications sent by the *Heart Rate Measurement* characteristic.

    ![](img/register_to_notifications.png)

    **figure 5** How to register to notifications using Master Control Panel 4.0.5


1. You should see the heart rate value change every half second. It begins at 100, goes up to 175 (in steps of 1), resets to 100 and so on.

    ![](img/notifications.png)

    **figure 6** Notifications view using Master Control Panel 4.0.5
