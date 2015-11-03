This example demonstrates how to use the Health Thermometer Service. The Health
Thermometer service reports two pieces of information, Temperature and Sensor
Location.

Further Technical Details can be found at the following links

[Temperature Service](https://developer.bluetooth.org/gatt/profiles/Pages/ProfileViewer.aspx?u=org.bluetooth.profile.health_thermometer.xml): Gatt profile details from bluetooth.org
[Temperature Measurement](https://developer.bluetooth.org/gatt/characteristics/Pages/CharacteristicViewer.aspx?u=org.bluetooth.characteristic.temperature_measurement.xml): Gatt Characteristic details
[Temperature Type](https://developer.bluetooth.org/gatt/characteristics/Pages/CharacteristicViewer.aspx?u=org.bluetooth.characteristic.temperature_type.xml): Gatt Characteristic details

Checking for Success
====================

Your Health Thermometer peripheral should be detectable by BLE scanners (e.g. a
smartphone). To use your phone as a BLE scanner simply install one of the
following apps:

- For Android, you can get [nRF Master Control Panel](https://play.google.com/store/apps/details?id=no.nordicsemi.android.mcp&hl=en).

- For iPhone, you can get [LightBlue](https://itunes.apple.com/gb/app/lightblue-bluetooth-low-energy/id557428110?mt=8).

Using the phone app you can connect to the peripheral and check the values of the
characteristics.
