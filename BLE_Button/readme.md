BLE_Button is a BLE service template. It handles a read-only characteristic with a simple input (boolean values). The input's source is the button on the board itself - the characteristic's value changes when the button is pressed or released.

The template covers:

1. Setting up advertising and connection modes.

1. Creating an input characteristic: read-only, boolean, with notifications.

1. Constructing a service class and adding it to the BLE stack.

1. Assigning UUIDs to the service and its characteristic.

1. Pushing notifications when the characteristic's value changes.

# Running the application

## Requirements

The sample application can be seen on any BLE scanner on a smartphone. If you don't have a scanner on your phone, please install :

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

1. Find your device; it should appear with the name `Button` in the scanner.

    ![](img/scan_results.png)

    **figure 2** Scan results using nRF Master Control Panel 4.0.5

1. Establish a connection with the device.

    ![](img/connection.png)

    **figure 3**  How to establish a connection using Master Control Panel 4.0.5

1. Discover the services and the characteristics on the device. The *Button service* has the UUID `0xA000` and includes the *Button state characteristic* which has the UUID `0xA001`. Depending on your scanner, non standard 16-bit UUID's can be displayed as 128-bit UUID's. If it is the case the following format will be used: `0000XXXX-0000-1000-8000-00805F9B34FB` where `XXXX` is the hexadecimal representation of the 16-bit UUID value.

    ![](img/discovery.png)

    **figure 4** Representation of the Button service using Master Control Panel 4.0.5

1. Register for the notifications sent by the button state characteristic then the scanner will automatically receive a notification containing the new state of the button every time the state of the button changes.

    ![](img/register_to_notifications.png)

    **figure 5** How to register to notifications using Master Control Panel 4.0.5


1. Pressing Button 1 on your board updates the state of the button and sends a notification to the scanner. The new state of the button characteristic value should be equal to 0x01.

    ![](img/button_pressed.png)

    **figure 6** Notification of button pressed using Master Control Panel 4.0.5

1. Releasing Button 1 on your board updates the state of the button and sends a notification to the scanner. The new state of the button characteristic value should be equal to 0x00.

    ![](img/button_depressed.png)

    **figure 7** Notification of button depressed using Master Control Panel 4.0.5

