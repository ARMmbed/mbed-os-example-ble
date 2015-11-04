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

/* We can arbiturarily choose the GAPButton service UUID to be 0xAA00
 * as long as it does not overlap with the UUIDs defined here:
 * https://developer.bluetooth.org/gatt/services/Pages/ServicesHome.aspx */
#define GAPButtonUUID 0xAA00
const uint16_t uuid16_list[] = {GAPButtonUUID};

uint8_t service_data[3];

void buttonPressedCallback(void)
{
    count++;
    // Update the count in the SERVICE_DATA field of the advertising payload
    service_data[2] = count;
    ble.gap().updateAdvertisingPayload(GapAdvertisingData::SERVICE_DATA, (uint8_t *)service_data, sizeof(service_data));
}

void blinkCallback(void)
{
    led1 = !led1;
}

void bleInitComplete(BLE::InitializationCompleteCallbackContext *)
{
    // Set up the advertising payload
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
    // Put the device name in the advertising payload
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME, (uint8_t *)DEVICE_NAME, sizeof(DEVICE_NAME));
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_16BIT_SERVICE_IDS, (uint8_t *)uuid16_list, sizeof(uuid16_list));

    // First two bytes of SERVICE_DATA field should contain the UUID of the service
    service_data[0] = GAPButtonUUID & 0xff;
    service_data[1] = GAPButtonUUID >> 8;
    service_data[2] = count; // Put the button click count in the third byte
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::SERVICE_DATA, (uint8_t *)service_data, sizeof(service_data));

    // It is not connectable as we are just boardcasting
    ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_NON_CONNECTABLE_UNDIRECTED);
    // Send out the advertising payload every 1000ms
    ble.gap().setAdvertisingInterval(1000);

    ble.gap().startAdvertising();
}

void app_start(int, char**)
{
    count = 0;
    ble.init(bleInitComplete);

    // Blink LED every 500 ms to indicate system aliveness
    minar::Scheduler::postCallback(blinkCallback).period(minar::milliseconds(500));

    // Register function to be called when button is released
    button.rise(buttonPressedCallback);
}
