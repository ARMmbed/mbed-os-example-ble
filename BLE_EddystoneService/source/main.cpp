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

#include "mbed-drivers/mbed.h"
#include "ble/BLE.h"
#include "EddystoneService.h"

#include "PersistentStorageHelper/ConfigParamsPersistence.h"

EddystoneService *eddyServicePtr;

/* Duration after power-on that config service is available. */
static const int CONFIG_ADVERTISEMENT_TIMEOUT_SECONDS = 30;

/* Default UID frame data */
static const UIDNamespaceID_t uidNamespaceID = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99};
static const UIDInstanceID_t  uidInstanceID  = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

/* Default version in TLM frame */
static const uint8_t tlmVersion = 0x00;

/* Values for ADV packets related to firmware levels, calibrated based on measured values at 1m */
static const PowerLevels_t defaultAdvPowerLevels = {-47, -33, -21, -13};
/* Values for radio power levels, provided by manufacturer. */
static const PowerLevels_t radioPowerLevels      = {-30, -16, -4, 4};

static EventQueue eventQueue(
    /* event count */ 16
);

DigitalOut led(LED1, 1);

/**
 * Callback triggered upon a disconnection event.
 */
static void disconnectionCallback(const Gap::DisconnectionCallbackParams_t *cbParams)
{
    (void) cbParams;
    BLE::Instance().gap().startAdvertising();
}

/**
 * Callback triggered some time after application started to switch to beacon mode.
 */
static void timeout(void)
{
    Gap::GapState_t state;
    state = BLE::Instance().gap().getState();
    if (!state.connected) { /* don't switch if we're in a connected state. */
        eddyServicePtr->startBeaconService();
        EddystoneService::EddystoneParams_t params;
        eddyServicePtr->getEddystoneParams(params);
        saveEddystoneServiceConfigParams(&params);
    } else {
        eventQueue.post_in(timeout, CONFIG_ADVERTISEMENT_TIMEOUT_SECONDS * 1000);
    }
}

static void blinky(void)
{
    led = !led;
}

static void onBleInitError(BLE::InitializationCompleteCallbackContext* initContext)
{
    /* Initialization error handling goes here... */
    (void) initContext;
}

static void initializeEddystoneToDefaults(BLE &ble)
{
    /* Set everything to defaults */
    eddyServicePtr = new EddystoneService(ble, defaultAdvPowerLevels, radioPowerLevels, eventQueue);

    /* Set default URL, UID and TLM frame data if not initialized through the config service */
    const char* url = YOTTA_CFG_EDDYSTONE_DEFAULT_URL;
    eddyServicePtr->setURLData(url);
    eddyServicePtr->setUIDData(uidNamespaceID, uidInstanceID);
    eddyServicePtr->setTLMData(tlmVersion);
}

static void bleInitComplete(BLE::InitializationCompleteCallbackContext* initContext)
{
    BLE         &ble  = initContext->ble;
    ble_error_t error = initContext->error;

    if (error != BLE_ERROR_NONE) {
        onBleInitError(initContext);
        return;
    }

    ble.gap().onDisconnection(disconnectionCallback);

    EddystoneService::EddystoneParams_t params;
    if (loadEddystoneServiceConfigParams(&params)) {
        eddyServicePtr = new EddystoneService(ble, params, radioPowerLevels, eventQueue);
    } else {
        initializeEddystoneToDefaults(ble);
    }

    /* Start Eddystone in config mode */
   eddyServicePtr->startConfigService();

   eventQueue.post_in(timeout, CONFIG_ADVERTISEMENT_TIMEOUT_SECONDS * 1000);
}

void scheduleBleEventsProcessing(BLE::OnEventsToProcessCallbackContext* context) {
    BLE &ble = BLE::Instance();
    eventQueue.post(Callback<void()>(&ble, &BLE::processEvents));
}


int main()
{
    /* Tell standard C library to not allocate large buffers for these streams */
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
    setbuf(stdin, NULL);

    eventQueue.post_every(blinky, 500);

    BLE &ble = BLE::Instance();
    ble.onEventsToProcess(scheduleBleEventsProcessing);
    ble.init(bleInitComplete);

    while (true) {
        eventQueue.dispatch();
    }
    return 0;
}
