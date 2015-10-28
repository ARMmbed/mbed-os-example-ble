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

const char DEVICE_NAME[] = "GapButton";

void buttonPressedCallback(void)
{
    count++;
    ble.gap().updateAdvertisingPayload(GapAdvertisingData::MANUFACTURER_SPECIFIC_DATA, &count, sizeof(count));
}

void blinkCallback(void)
{
    led1 = !led1; /* Do blinky on LED1 to indicate system aliveness. */
}

void app_start(int, char**)
{
    ble.init();

    /* setup advertising */
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME, (uint8_t *)DEVICE_NAME, sizeof(DEVICE_NAME));
    ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_NON_CONNECTABLE_UNDIRECTED);
    ble.gap().setAdvertisingInterval(1000); /* 1000ms. */

    count = 0;
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::MANUFACTURER_SPECIFIC_DATA, &count, sizeof(count));
    minar::Scheduler::postCallback(blinkCallback).period(minar::milliseconds(500));
    button.rise(buttonPressedCallback);

    ble.gap().startAdvertising();
}
