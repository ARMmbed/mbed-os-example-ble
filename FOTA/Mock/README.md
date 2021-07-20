# FOTA service Mock example

The FOTA service, as defined in its [specification](https://github.com/ARMmbed/mbed-os-experimental-ble-services/blob/fota-service-github-ci/services/FOTA/docs/README.md) document, facilitates the transfer of firmware updates over BLE.

In this demo, the FOTA service is used to transfer a binary from the host PC into the flash of the target. 
A basic _FOTA client_, implemented in Python using [bleak](https://pypi.org/project/bleak/), is used to read/write the binary stream, control and status characteristics. 
Please refer to [Section 1](https://github.com/ARMmbed/mbed-os-experimental-ble-services/tree/fota-service-github-ci/services/FOTA/docs#fota-service-structure) of the spec to learn more about these characteristics.

To verify the success of the transfer, the application computes the [SHA-256](https://en.wikipedia.org/wiki/SHA-2) of the binary and prints it to the serial port.
The SHA-256 is also computed on the host using [sha256sum](https://man7.org/linux/man-pages/man1/sha256sum.1.html).
The transfer is regarded as a success if the hashes are equal.

## Usage

### Hardware requirements

This application requires either the [DISCO_L475VG_IOT01A](https://os.mbed.com/platforms/ST-Discovery-L475E-IOT01A/) or [NRF52840_DK](https://os.mbed.com/platforms/Nordic-nRF52-DK/) platforms.

To use the Python client, the host PC must have Bluetooth capabilities, either through an inbuilt chipset or an external USB adapter.

### Build instructions 

#### Setting up the environment

1. Create a Python virtual environment inside the client folder:

   ```shell
   cd client && mkdir venv && virtualenv venv && source venv/bin/activate && python -m pip install --upgrade pip
   ```

1. Import the missing dependencies in the target folder:

   ```shell
   cd ../target && mbed-tools deploy
   ```

1. Install the Mbed OS requirements:

   ```shell
   pip install -r mbed-os/requirements.txt
   ```

1. Install mbed-tools and bleak:

   ```shell
   pip install mbed-cli bleak intelhex
   ```

#### Building the application

1. The application can be built and flashed onto the target using Mbed CLI 2:

   ```shell
   mbed-tools compile -t <toolchain> -m <target> -f
   ```

1. A `bin` file is required for the demo. 
   For nRF, this must be created manually from the `elf` as Mbed Tools outputs a `hex` file by default:

   ```shell
   arm-none-eabi-objcopy -O binary cmake_build/NRF52840_DK/develop/<toolchain>/BLE_GattServer_FOTAService.elf cmake_build/NRF52840_DK/develop/<toolchain>/BLE_GattServer_FOTAService.bin
   ```

### Demonstration

1. Open a serial terminal on the host PC to receive serial prints from the _FOTA target_: 
   
   ```shell 
   mbed term -b 115200
   ```

1. In a separate window, run the test script: 
   
   ```
   python client/client.py
   ```
   
   It scans for a device named 'FOTA' and attempts to connect to it.
   Once connected, it asks the user to enter the path to the binary.
   Use the binary running on the target:
   
   ```
   Enter the path to the binary: ../cmake_build/<target>/develop/<toolchain>/BLE_GattServer_FOTAService.bin
   ```
   
1. The client initiates the transfer once the FOTA session begins and commits the update once the entire binary has been sent.
   Subsequently, the target computes the SHA-256 of the binary and prints it to the serial.
   Verify the `<hash>` with sha256sum:
   
   ```shell
   echo "<hash> ../cmake_build/<target>/develop/<toolchain>/BLE_GattServer_FOTAService.bin" | sha256sum --check
   ```
