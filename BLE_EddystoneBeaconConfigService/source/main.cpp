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

#include "mbed-drivers/mbed.h"
#include "ble/BLE.h"
#include "ble/services/EddystoneConfigService.h"
#include "ConfigParamsPersistence.h"

BLE ble;
EddystoneConfigService *EddystoneBeaconConfig;
EddystoneConfigService::Params_t params;

/**
 * URIBeaconConfig service can operate in two modes: a configuration mode which
 * allows a user to update settings over a connection; and normal URIBeacon mode
 * which involves advertising a URI. Constructing an object from URIBeaconConfig
 * service sets up advertisements for the configuration mode. It is then up to
 * the application to switch to URIBeacon mode based on some timeout.
 *
 * The following help with this switch.
 */
static const int CONFIG_ADVERTISEMENT_TIMEOUT_SECONDS = 30;  // Duration after power-on that config service is available.

/**
 * Stop advertising the Config Service after a delay; and switch to a non-connectable advertising mode only beacon.
 */
void timeout(void)
{
    Gap::GapState_t state;
    state = ble.gap().getState();
    if (!state.connected) { /* don't switch if we're in a connected state. */
        EddystoneBeaconConfig->setupEddystoneAdvertisements();
    } else {
         minar::Scheduler::postCallback(timeout).delay(minar::milliseconds(CONFIG_ADVERTISEMENT_TIMEOUT_SECONDS * 1000));
    }
}

/**
 * Callback triggered upon a disconnection event.
 */
void disconnectionCallback(const Gap::DisconnectionCallbackParams_t *cbParams)
{
    if (true == params.isConfigured){
        /* end advertising, the beacon is configured */
        timeout();
    } else{
        /* eddystone is not configured, continue advertising */
        ble.gap().startAdvertising();
    }
}

/**
 * This function is called when the ble initialization process has failled
 */
void onBleInitError(BLE &ble, ble_error_t error)
{
    /* Initialization error handling should go here */
}

/**
 * Callback triggered when the ble initialization process has finished
 */
void bleInitComplete(BLE::InitializationCompleteCallbackContext *initContext)
{
    BLE&        ble   = initContext->ble;
    ble_error_t error = initContext->error;

    if (error != BLE_ERROR_NONE) {
        /* In case of error, forward the error handling to onBleInitError */
        onBleInitError(ble, error);
        return;
    }

    /* Ensure that it is the default instance of BLE */
    if(ble.getInstanceID() != BLE::DEFAULT_INSTANCE) {
        return;
    }

    ble.gap().onDisconnection(disconnectionCallback);

    /*
     * Load parameters from (platform specific) persistent storage. Parameters
     * can be set to non-default values while the URIBeacon is in configuration
     * mode (within the first 60 seconds of power-up). Thereafter, parameters
     * get copied out to persistent storage before switching to normal URIBeacon
     * operation.
     */
    bool fetchedFromPersistentStorage = loadURIBeaconConfigParams(&params);

    /* Set UID and TLM frame data */
    EddystoneConfigService::UIDNamespaceID_t uidNamespaceID = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99}; // 10Byte Namespace UUID
    EddystoneConfigService::UIDInstanceID_t  uidInstanceID = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}; // 6Byte Instance ID
    uint8_t tlmVersion = 0x00;

    /* Initialize a EddystoneBeaconConfig service providing config params, default URI, and power levels. */
    static EddystoneConfigService::PowerLevels_t defaultAdvPowerLevels = {-47, -33, -21, -13}; // Values for ADV packets related to firmware levels, calibrated based on measured values at 1m
    static EddystoneConfigService::PowerLevels_t radioPowerLevels      = {-30, -16, -4, 4};    // Values for radio power levels, provided by manufacturer.

    /* Create Eddystone Config Service object */
    EddystoneBeaconConfig = new EddystoneConfigService(ble, params, defaultAdvPowerLevels, radioPowerLevels);

    /* Set default URI, UID and TLM frame data if not initialized through the config service */
    EddystoneBeaconConfig->setDefaultURIFrameData("http://mbed.org", 2);
    EddystoneBeaconConfig->setDefaultUIDFrameData(&uidNamespaceID, &uidInstanceID,5);
    EddystoneBeaconConfig->setDefaultTLMFrameData(tlmVersion, 10);

    /* start the config service */
    EddystoneBeaconConfig->start(!fetchedFromPersistentStorage);

    if (!EddystoneBeaconConfig->initSuccessfully()) {
        ::error("failed to accommodate URI");
    }
    /* Post a timeout callback to be invoked in CONFIG_ADVERTISEMENT_TIMEOUT_SECONDS to affect the switch to beacon mode. */
    minar::Scheduler::postCallback(timeout).delay(minar::milliseconds(CONFIG_ADVERTISEMENT_TIMEOUT_SECONDS * 1000));

    ble.gap().startAdvertising(); /* Set the whole thing in motion. After this call a GAP central can scan the EddystoneBeaconConfig
                                   * service. This can then be switched to the normal URIBeacon functionality after a timeout. */
}

void app_start(int, char *[])
{
    ble.init(bleInitComplete);
}
