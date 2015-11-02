/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mbed-drivers/mbed.h"
#include "ble/BLE.h"

BLE ble;
DigitalOut  led1(LED1, 1);
InterruptIn button(BUTTON1);
uint8_t count;

// Change your device name below
const char DEVICE_NAME[] = "GAPButton";

// This function is called when the button is released
void buttonPressedCallback(void)
{
    count++;
    // Modify the BLE advertising payload
    ble.gap().updateAdvertisingPayload(GapAdvertisingData::MANUFACTURER_SPECIFIC_DATA, &count, sizeof(count));
}

// Do blinky on LED1 to indicate system aliveness
void blinkCallback(void)
{
    led1 = !led1;
}

void app_start(int, char**)
{
    // Initialise count
    count = 0;
    // Initialise BLE stack
    ble.init();

    // Set up advertising
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
    // Put the device name in the advertising payload
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME, (uint8_t *)DEVICE_NAME, sizeof(DEVICE_NAME));
    // Broadcast the value of count in the MANUFACTURER_SPECIFIC_DATA field
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::MANUFACTURER_SPECIFIC_DATA, &count, sizeof(count));
    // It is not connectable as we are just boardcasting
    ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_NON_CONNECTABLE_UNDIRECTED);
    // Send out the advertising payload every 1000ms
    ble.gap().setAdvertisingInterval(1000);

    // Blink LED every 500 ms
    minar::Scheduler::postCallback(blinkCallback).period(minar::milliseconds(500));
    // Register callback function to be called when button is released
    button.rise(buttonPressedCallback);

    // Start advertising
    ble.gap().startAdvertising();
}
