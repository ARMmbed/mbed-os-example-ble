# FOTA Service Mock Example

**Note:** These are basic instructions intended for reviewers. A more comprehensive README is required. 

## Usage
1. Using Mbed CLI (CMake is WIP), build and flash the FOTA target onto the board.

    ```shell
    mbed compile -t GCC_ARM -m NRF52840_DK -f 
    ```

1. CLI 1 outputs a hex file to the BUILD folder, but we need a bin. Manually create it from the elf using objcopy:

    ```shell
    arm-none-eabi-objcopy -O binary BUILD/NRF52840_DK/GCC_ARM/BLE_GattServer_FOTAService.elf BUILD/NRF52840_DK/GCC_ARM/BLE_GattServer_FOTAService.bin
    ```
   The file should be approx. 325 KiB in size.

1. Create a Python virtual environment and activate it. Then, install [bleak](https://pypi.org/project/bleak/).

   ```shell
   cd scripts && mkdir venv && virtualenv venv && source venv/bin/activate  
   pip install bleak==0.10.0  
   ```
   
4. Run the test script. It will guide you through the process of transferring the binary.

   ```shell
   ./test_fota.py
   ```

5. Once you commit the update, the target will compute its SHA256 and print it to the serial. Verify the hash with sha256sum.

   ```shell
   sha256sum BUILD/NRF52840_DK/GCC_ARM/BLE_GattServer_FOTAService.bin
   ```
