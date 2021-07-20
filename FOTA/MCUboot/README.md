# FOTA service MCUboot example

This application extends the [Mock](../Mock) example by including a bootloader based on MCUboot.

## Usage

### Hardware requirements
This application requires the [NRF52840_DK](https://os.mbed.com/platforms/Nordic-nRF52-DK/) platform.

To use the Python client, the host PC must have Bluetooth capabilities, either through an inbuilt chipset or an external USB adapter.

### Build instructions

#### Setting up the environment

1. Create a Python virtual environment inside the client folder:

   ```shell
   cd client && mkdir venv && virtualenv venv && source venv/bin/activate && python -m pip install --upgrade pip
   ```
   
1. Import the missing dependencies in the application and bootloader folders:
   
   ```shell
   cd ../target/application && mbed deploy && cd ../bootloader && mbed deploy
   ```

1. Install the Mbed OS and MCUboot requirements. 
   Then, run the MCUboot setup script:

   ```shell
   pip install -r mbed-os/requirements.txt -r mcuboot/scripts/requirements.txt && python mcuboot/scripts/setup.py install
   ```

1. Install mbed-cli, bleak and intelhex

   ```shell
   pip install mbed-cli bleak intelhex
   ```

#### Creating the signing keys and building the bootloader

1. Generate a [RSA-2048](https://en.wikipedia.org/wiki/RSA_numbers#RSA-2048) key pair:

   ```shell
   mcuboot/scripts/imgtool.py keygen -k signing-keys.pem -t rsa-2048
   ```

1. Extract the public key into a C data structure so that it can be built into the bootloader:

   ```shell
   mcuboot/scripts/imgtool.py getpub -k signing-keys.pem >> signing_keys.c
   ```

1. Build the bootloader:

   ```shell
   mbed compile -t GCC_ARM -m NRF52840_DK
   ```

#### Building and signing the primary application

1. Build the primary application:

   ```shell
   cd ../application && mbed compile -t GCC_ARM -m NRF52840_DK
   ```

1. Copy the HEX file into the bootloader folder:

   ```shell
   cp BUILD/NRF52840_DK/GCC_ARM/application.hex ../bootloader && cd ../bootloader
   ```

1. Sign the initial application using the rsa-2048 keys:

   ```shell
   mcuboot/scripts/imgtool.py sign -k signing-keys.pem --align 4 -v 0.1.0 --header-size 4096 --pad-header -S 0xC0000 --pad application.hex signed_application.hex
   ```
   
#### Creating and flashing the "factory firmware"

1. Merge the bootloader and signed application:

   ```shell
   hexmerge.py -o merged.hex --no-start-addr BUILD/NRF52840_DK/GCC_ARM/bootloader.hex signed_application.hex
   ```

1. Erase the chip:
   
   ```shell
   pyocd erase --chip
   ```
   
1. Flash the "factory firmware" onto the board by copying `merged.hex` over to its mount point:

   ```shell
   cp merged.hex <mount_point>
   ```
   
   The target's mount point can be found by running `mbedls`.
   
#### Creating the update binary

1. Change the application's version number in its [mbed_app.json](target/application/mbed_app.json) to 0.1.1 and rebuild it:

   ```shell
   cd ../application && mbed compile -t GCC_ARM -m NRF52840_DK
   ```

1. Copy the HEX file into the bootloader folder:

   ```shell
   cp BUILD/NRF52840_DK/GCC_ARM/application.hex ../bootloader && cd ../bootloader
   ```

1. Sign the update application using the rsa-2048 keys:

   ```shell
   mcuboot/scripts/imgtool.py sign -k signing-keys.pem --align 4 -v 0.1.1 --header-size 4096 --pad-header -S 0x55000 application.hex signed_update.hex
   ```

1. Generate a raw binary file from `signed_update.hex` so that it can be transported over BLE:

   ```shell
   arm-none-eabi-objcopy -I ihex -O binary signed_update.hex signed_update.bin
   ```

### Demonstration

1. Open a serial terminal on the host PC to receive serial prints from the target:

   ```shell 
   mbed term -b 115200
   ```

2. In a separate window, run the client script:

   ```shell
   python client/client.py
   ```

   It scans for a device named 'FOTA' and attempts to connect to it.
   Once connected, it asks the user to enter the path to the binary.
   Enter the signed update binary:

   ```
   Enter the path to the binary: target/bootloader/signed_update.bin
   ```
   
   Note the firmware revision of the application running on the device:

   ```
   xxxx-xx-xx xx:xx:xx,xxx - logger - INFO - DFU Service found with firmware rev 0.1.0 for device "Primary MCU"
   ```
   
1. The client initiates the transfer once the FOTA session begins and commits the update once the entire binary has been sent.
   Subsequently, the target sets the update as pending and initiates a system reboot.
   Once the new application boots, the client reconnects and rereads the Firmware Revision Characteristic:
   
   ```
   xxxx-xx-xx xx:xx:xx,xxx - logger - INFO - DFU Service found with firmware rev 0.1.1 for device "Primary MCU"
   ```
