# Button count over GAP

This application shows how to use GAP to transmit a simple value to disconnected peer listening for advertisement every time that a value is updated:

1. The value is a count of how many times a button on the device was pressed (the code actually monitors the button's releases, not press downs).

1. We transmit the value in the SERVICE_DATA field of the advertising payload.

# Running the application

## Requirements

The sample application can be seen on any BLE scanner on a smartphone. If you don't have a scanner on your phone, please install :

- [nRF Master Control Panel](https://play.google.com/store/apps/details?id=no.nordicsemi.android.mcp) for Android.

- [LightBlue](https://itunes.apple.com/gb/app/lightblue-bluetooth-low-energy/id557428110?mt=8) for iPhone.

Hardware requirements are in the [main readme](https://github.com/ARMmbed/ble-examples/blob/master/README.md).

## Building instructions

Building instructions for all samples are in the [main readme](https://github.com/ARMmbed/ble-examples/blob/master/README.md).

## Checking for success

**Note:** Screens captures depicted below show what is expected from this example if the scanner used is *nRF Master Control Panel* version 4.0.5. If you encounter any difficulties consider trying another scanner or another version of nRF Master Control Panel. Alternative scanners may require reference to their manuals.

1. Build the application and install it on your board as explained in the building instructions.

1. Open the BLE scanner on your phone.

1. Start a scan.

    ![](img/start_scan.png)

    **figure 1** How to start scan using nRF Master Control Panel 4.0.5.

1. Find your device; it should be named `GAPButton`; and look at the advertisement broadcasted by your device (there is no need to connect to your device).

    ![](img/discovery.png)

    **figure 2** Scan results using nRF Master Control Panel 4.0.5.

1. The Service Data field of the advertisement packet broadcasted by your device reflects the button press count. The starting value is 0.

    ![](img/initial_state.png)

    **figure 3** Initial state of the button using nRF Master Control Panel 4.0.5.

1. Press the button on the device.

    ![](img/first_press.png)

    **figure 3** State after 1 button press using nRF Master Control Panel 4.0.5.

1. The Service Data field value of the advertisement packet should change every time you press the button.

    ![](img/result.png)

    **figure 3** State after 6 button press using nRF Master Control Panel 4.0.5.

## Note

Since broadcasting is not reliable and your phone may scan intermittently, it is possible that your phone will miss button updates.
