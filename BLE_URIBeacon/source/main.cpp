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
#include "ble/services/URIBeaconConfigService.h"
#include "ble/services/DFUService.h"
#include "ble/services/DeviceInformationService.h"
#include "ConfigParamsPersistence.h"

/**
 * URIBeaconConfig service can operate in two modes: a configuration mode which
 * allows a user to update settings over a connection; and normal URIBeacon mode
 * which involves advertising a URI. Constructing an object from URIBeaconConfig
 * service sets up advertisements for the configuration mode. It is then up to
 * the application to switch to URIBeacon mode based on some timeout.
 *
 * The following help with this switch.
 */
static const int CONFIG_ADVERTISEMENT_TIMEOUT_SECONDS = 60;  // Duration after power-on that config service is available.

static EventQueue eventQueue(
    /* event count */ 16 * /* event size */ 32    
);

/* global static objects */
BLE ble;
URIBeaconConfigService *uriBeaconConfig;
URIBeaconConfigService::Params_t params;

/**
 * Stop advertising the UriBeaconConfig Service after a delay; and switch to normal URIBeacon.
 */
void timeout(void)
{
    Gap::GapState_t state;
    state = ble.getGapState();
    if (!state.connected) { /* don't switch if we're in a connected state. */
        uriBeaconConfig->setupURIBeaconAdvertisements();
        ble.startAdvertising();
    } else {
        eventQueue.post_in(timeout, CONFIG_ADVERTISEMENT_TIMEOUT_SECONDS * 1000);
    }
}

/**
 * Callback triggered upon a disconnection event. Needs to re-enable advertisements.
 */
void disconnectionCallback(const Gap::DisconnectionCallbackParams_t *)
{
    ble.startAdvertising();
}

void scheduleBleEventsProcessing(BLE::OnEventsToProcessCallbackContext* context) {
    BLE &ble = BLE::Instance();
    eventQueue.post(Callback<void()>(&ble, &BLE::processEvents));
}

int main()
{
    ble.onEventsToProcess(scheduleBleEventsProcessing);
    ble.init();
    ble.onDisconnection(disconnectionCallback);

    /*
     * Load parameters from (platform specific) persistent storage. Parameters
     * can be set to non-default values while the URIBeacon is in configuration
     * mode (within the first 60 seconds of power-up). Thereafter, parameters
     * get copied out to persistent storage before switching to normal URIBeacon
     * operation.
     */
    bool fetchedFromPersistentStorage = loadURIBeaconConfigParams(&params);

    /* Initialize a URIBeaconConfig service providing config params, default URI, and power levels. */
    static URIBeaconConfigService::PowerLevels_t defaultAdvPowerLevels = {-20, -4, 0, 10}; // Values for ADV packets related to firmware levels
    uriBeaconConfig = new URIBeaconConfigService(ble, params, !fetchedFromPersistentStorage, "http://uribeacon.org", defaultAdvPowerLevels);
    if (!uriBeaconConfig->configuredSuccessfully()) {
        error("failed to accommodate URI");
    }

    // Setup auxiliary services to allow over-the-air firmware updates, etc
    DFUService *dfu = new DFUService(ble);
    DeviceInformationService *deviceInfo = new DeviceInformationService(ble, "ARM", "UriBeacon", "SN1", "hw-rev1", "fw-rev1", "soft-rev1");

    ble.startAdvertising(); /* Set the whole thing in motion. After this call a GAP central can scan the URIBeaconConfig
                             * service. This can then be switched to the normal URIBeacon functionality after a timeout. */

    eventQueue.post_in(timeout, CONFIG_ADVERTISEMENT_TIMEOUT_SECONDS * 1000);

    while (true) {
        eventQueue.dispatch();
    }

    return 0;
}
