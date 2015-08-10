/* mbed Microcontroller Library
 * Copyright (c) 2006-2015 ARM Limited
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

#include "mbed.h"
#include "ble/BLE.h"
#include "ble/services/HeartRateService.h"

BLE        ble;
DigitalOut led1(LED1, 1);

const static char     DEVICE_NAME[] = "HRM";
static const uint16_t uuid16_list[] = {GattService::UUID_HEART_RATE_SERVICE};

static uint8_t hrmCounter = 100; // init HRM to 100bps
static HeartRateService *hrServicePtr;

void disconnectionCallback(Gap::Handle_t, Gap::DisconnectionReason_t)
{
    ble.gap().startAdvertising(); // restart advertising
}

void updateSensorValue() {
    // Do blocking calls or whatever is necessary for sensor polling.
    // In our case, we simply update the HRM measurement.
    hrmCounter++;

    //  100 <= HRM bps <=175
    if (hrmCounter == 175) {
        hrmCounter = 100;
    }

    hrServicePtr->updateHeartRate(hrmCounter);
}

void periodicCallback(void)
{
    led1 = !led1; /* Do blinky on LED1 while we're waiting for BLE events */

    if (ble.getGapState().connected) {
        minar::Scheduler::postCallback(updateSensorValue);
    }
}

void app_start(int argc, char *argv[])
{
    minar::Scheduler::postCallback(periodicCallback).period(minar::milliseconds(500));

    ble.init();
    ble.gap().onDisconnection(disconnectionCallback);

    /* Setup primary service. */
    hrServicePtr = new HeartRateService(ble, hrmCounter, HeartRateService::LOCATION_FINGER);

    /* Setup advertising. */
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_16BIT_SERVICE_IDS, (uint8_t *)uuid16_list, sizeof(uuid16_list));
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::GENERIC_HEART_RATE_SENSOR);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME, (uint8_t *)DEVICE_NAME, sizeof(DEVICE_NAME));
    ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    ble.gap().setAdvertisingInterval(1000); /* 1000ms */
    ble.gap().startAdvertising();
}
