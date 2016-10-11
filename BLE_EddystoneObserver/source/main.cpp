/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <mbed-events/events.h>
#include "mbed.h"
#include "ble/BLE.h"

static const int URI_MAX_LENGTH = 18;             // Maximum size of service data in ADV packets

static EventQueue eventQueue(
    /* event count */ 16 * /* event size */ 32
);

DigitalOut led1(LED1, 1);

void periodicCallback(void)
{
    led1 = !led1; /* Do blinky on LED1 while we're waiting for BLE events */
}

void decodeURI(const uint8_t* uriData, const size_t uriLen)
{
    const char *prefixes[] = {
        "http://www.",
        "https://www.",
        "http://",
        "https://",
        "urn:uuid:"
    };
    const size_t NUM_PREFIXES = sizeof(prefixes) / sizeof(char *);
    const char *suffixes[] = {
        ".com/",
        ".org/",
        ".edu/",
        ".net/",
        ".info/",
        ".biz/",
        ".gov/",
        ".com",
        ".org",
        ".edu",
        ".net",
        ".info",
        ".biz",
        ".gov"
    };
    const size_t NUM_SUFFIXES = sizeof(suffixes) / sizeof(char *);

    size_t index = 0;

    /* First byte is the URL Scheme. */
    if (uriData[index] < NUM_PREFIXES) {
        printf("%s", prefixes[uriData[index]]);
        index++;
    } else {
        printf("URL Scheme was not encoded!");
        return;
    }

    /* From second byte onwards we can have a character or a suffix */
    while(index < uriLen) {
        if (uriData[index] < NUM_SUFFIXES) {
            printf("%s", suffixes[uriData[index]]);
        } else {
            printf("%c", uriData[index]);
        }
        index++;
    }

    printf("\n\r");
}

/*
 * This function is called every time we scan an advertisement.
 */
void advertisementCallback(const Gap::AdvertisementCallbackParams_t *params)
{
    struct AdvertisingData_t {
        uint8_t                        length; /* doesn't include itself */
        GapAdvertisingData::DataType_t dataType;
        uint8_t                        data[1];
    } AdvDataPacket;

    struct ApplicationData_t {
        uint8_t applicationSpecificId[2];
        uint8_t frameType;
        uint8_t advPowerLevels;
        uint8_t uriData[URI_MAX_LENGTH];
    } AppDataPacket;

    const uint8_t BEACON_UUID[sizeof(UUID::ShortUUIDBytes_t)] = {0xAA, 0xFE};
    const uint8_t FRAME_TYPE_URL                              = 0x10;
    const uint8_t APPLICATION_DATA_OFFSET                     = sizeof(ApplicationData_t) + sizeof(AdvDataPacket.dataType) - sizeof(AppDataPacket.uriData);

    AdvertisingData_t *pAdvData;
    size_t index = 0;
    while(index < params->advertisingDataLen) {
        pAdvData = (AdvertisingData_t *)&params->advertisingData[index];
        if (pAdvData->dataType == GapAdvertisingData::SERVICE_DATA) {
            ApplicationData_t *pAppData = (ApplicationData_t *) pAdvData->data;
            if (!memcmp(pAppData->applicationSpecificId, BEACON_UUID, sizeof(BEACON_UUID)) && (pAppData->frameType == FRAME_TYPE_URL)) {
                decodeURI(pAppData->uriData, pAdvData->length - APPLICATION_DATA_OFFSET);
                break;
            }
        }
        index += (pAdvData->length + 1);
    }
}

void onBleInitError(BLE &ble, ble_error_t error)
{
   /* Initialization error handling should go here */
}

void bleInitComplete(BLE::InitializationCompleteCallbackContext *params)
{
    BLE&        ble   = params->ble;
    ble_error_t error = params->error;

    if (error != BLE_ERROR_NONE) {
        onBleInitError(ble, error);
        return;
    }

    if (ble.getInstanceID() != BLE::DEFAULT_INSTANCE) {
        return;
    }

    ble.gap().setScanParams(1800 /* scan interval */, 1500 /* scan window */);
    ble.gap().startScan(advertisementCallback);
}

void scheduleBleEventsProcessing(BLE::OnEventsToProcessCallbackContext* context) {
    BLE &ble = BLE::Instance();
    eventQueue.post(Callback<void()>(&ble, &BLE::processEvents));
}

int main()
{
    eventQueue.post_every(500, periodicCallback);

    BLE &ble = BLE::Instance();
    ble.onEventsToProcess(scheduleBleEventsProcessing);
    ble.init(bleInitComplete);

    while (true) {
        eventQueue.dispatch();
    }

    return 0;
}
