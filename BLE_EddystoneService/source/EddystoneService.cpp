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

#include "EddystoneService.h"

/* Initialise the EddystoneService using parameters from persistent storage */
EddystoneService::EddystoneService(BLE                 &bleIn,
                                   EddystoneParams_t   &paramsIn,
                                   const PowerLevels_t &radioPowerLevelsIn,
                                   EventQueue          &evQ,
                                   uint32_t            advConfigIntervalIn) :
    ble(bleIn),
    operationMode(EDDYSTONE_MODE_NONE),
    urlFrame(paramsIn.urlData, paramsIn.urlDataLength),
    uidFrame(paramsIn.uidNamespaceID, paramsIn.uidInstanceID),
    tlmFrame(paramsIn.tlmVersion),
    resetFlag(false),
    rawUrlFrame(NULL),
    rawUidFrame(NULL),
    rawTlmFrame(NULL),
    tlmBatteryVoltageCallback(NULL),
    tlmBeaconTemperatureCallback(NULL),
    uidFrameCallbackHandle(),
    urlFrameCallbackHandle(),
    tlmFrameCallbackHandle(),
    radioManagerCallbackHandle(),
    deviceName(DEFAULT_DEVICE_NAME),
    eventQueue(evQ)
{
    lockState      = paramsIn.lockState;
    flags          = paramsIn.flags;
    txPowerMode    = paramsIn.txPowerMode;
    urlFramePeriod = correctAdvertisementPeriod(paramsIn.urlFramePeriod);
    uidFramePeriod = correctAdvertisementPeriod(paramsIn.uidFramePeriod);
    tlmFramePeriod = correctAdvertisementPeriod(paramsIn.tlmFramePeriod);

    memcpy(lock,   paramsIn.lock,   sizeof(Lock_t));
    memcpy(unlock, paramsIn.unlock, sizeof(Lock_t));

    eddystoneConstructorHelper(paramsIn.advPowerLevels, radioPowerLevelsIn, advConfigIntervalIn);
}

/* When using this constructor we need to call setURLData,
 * setTMLData and setUIDData to initialise values manually
 */
EddystoneService::EddystoneService(BLE                 &bleIn,
                                   const PowerLevels_t &advPowerLevelsIn,
                                   const PowerLevels_t &radioPowerLevelsIn,
                                   EventQueue          &evQ,
                                   uint32_t            advConfigIntervalIn) :
    ble(bleIn),
    operationMode(EDDYSTONE_MODE_NONE),
    urlFrame(),
    uidFrame(),
    tlmFrame(),
    lockState(false),
    resetFlag(false),
    lock(),
    unlock(),
    flags(0),
    txPowerMode(0),
    urlFramePeriod(DEFAULT_URL_FRAME_PERIOD_MSEC),
    uidFramePeriod(DEFAULT_UID_FRAME_PERIOD_MSEC),
    tlmFramePeriod(DEFAULT_TLM_FRAME_PERIOD_MSEC),
    rawUrlFrame(NULL),
    rawUidFrame(NULL),
    rawTlmFrame(NULL),
    tlmBatteryVoltageCallback(NULL),
    tlmBeaconTemperatureCallback(NULL),
    uidFrameCallbackHandle(),
    urlFrameCallbackHandle(),
    tlmFrameCallbackHandle(),
    radioManagerCallbackHandle(),
    deviceName(DEFAULT_DEVICE_NAME),
    eventQueue(evQ)
{
    eddystoneConstructorHelper(advPowerLevelsIn, radioPowerLevelsIn, advConfigIntervalIn);
}

/* Setup callback to update BatteryVoltage in TLM frame */
void EddystoneService::onTLMBatteryVoltageUpdate(TlmUpdateCallback_t tlmBatteryVoltageCallbackIn)
{
    tlmBatteryVoltageCallback = tlmBatteryVoltageCallbackIn;
}

/* Setup callback to update BeaconTemperature in TLM frame */
void EddystoneService::onTLMBeaconTemperatureUpdate(TlmUpdateCallback_t tlmBeaconTemperatureCallbackIn)
{
    tlmBeaconTemperatureCallback = tlmBeaconTemperatureCallbackIn;
}

void EddystoneService::setTLMData(uint8_t tlmVersionIn)
{
   tlmFrame.setTLMData(tlmVersionIn);
}

void EddystoneService::setURLData(const char *urlDataIn)
{
    urlFrame.setURLData(urlDataIn);
}

void EddystoneService::setUIDData(const UIDNamespaceID_t &uidNamespaceIDIn, const UIDInstanceID_t &uidInstanceIDIn)
{
    uidFrame.setUIDData(uidNamespaceIDIn, uidInstanceIDIn);
}

EddystoneService::EddystoneError_t EddystoneService::startConfigService(void)
{
    if (operationMode == EDDYSTONE_MODE_CONFIG) {
        /* Nothing to do, we are already in config mode */
        return EDDYSTONE_ERROR_NONE;
    } else if (advConfigInterval == 0) {
        /* Nothing to do, the advertisement interval is 0 */
        return EDDYSTONE_ERROR_INVALID_ADVERTISING_INTERVAL;
    }

    if (operationMode == EDDYSTONE_MODE_BEACON) {
        ble.shutdown();
        stopBeaconService();
    }

    if (!ble.hasInitialized()) {
        operationMode = EDDYSTONE_MODE_CONFIG;
        ble.init(this, &EddystoneService::bleInitComplete);
        /* Set the device name once more */
        ble.gap().setDeviceName(reinterpret_cast<const uint8_t *>(deviceName));
        return EDDYSTONE_ERROR_NONE;
    }

    operationMode = EDDYSTONE_MODE_CONFIG;
    setupConfigService();
    return EDDYSTONE_ERROR_NONE;
}

EddystoneService::EddystoneError_t EddystoneService::startBeaconService(void)
{
    if (operationMode == EDDYSTONE_MODE_BEACON) {
        /* Nothing to do, we are already in beacon mode */
        return EDDYSTONE_ERROR_NONE;
    } else if (!urlFramePeriod && !uidFramePeriod && !tlmFramePeriod) {
        /* Nothing to do, the period is 0 for all frames */
        return EDDYSTONE_ERROR_INVALID_ADVERTISING_INTERVAL;
    }

    if (operationMode == EDDYSTONE_MODE_CONFIG) {
        ble.shutdown();
        /* Free unused memory */
        freeConfigCharacteristics();
    }

    if (!ble.hasInitialized()) {
        operationMode = EDDYSTONE_MODE_BEACON;
        ble.init(this, &EddystoneService::bleInitComplete);
        /* Set the device name once more */
        ble.gap().setDeviceName(reinterpret_cast<const uint8_t *>(deviceName));
        return EDDYSTONE_ERROR_NONE;
    }

    operationMode = EDDYSTONE_MODE_BEACON;
    setupBeaconService();

    return EDDYSTONE_ERROR_NONE;
}

EddystoneService::EddystoneError_t EddystoneService::stopCurrentService(void)
{
    switch (operationMode) {
    case EDDYSTONE_MODE_NONE:
        return EDDYSTONE_ERROR_INVALID_STATE;
    case EDDYSTONE_MODE_BEACON:
        ble.shutdown();
        stopBeaconService();
        break;
    case EDDYSTONE_MODE_CONFIG:
        ble.shutdown();
        freeConfigCharacteristics();
        break;
    default:
        /* Some error occurred */
        error("Invalid EddystonService mode");
        break;
    }
    operationMode = EDDYSTONE_MODE_NONE;
    /* Currently on some platforms, the BLE stack handles power management,
     * so we should bring it up again, but not configure it.
     * Once the system sleep without BLE initialised is fixed, remove this
     */
    ble.init(this, &EddystoneService::bleInitComplete);

    return EDDYSTONE_ERROR_NONE;
}

ble_error_t EddystoneService::setCompleteDeviceName(const char *deviceNameIn)
{
    /* Make sure the device name is safe */
    ble_error_t error = ble.gap().setDeviceName(reinterpret_cast<const uint8_t *>(deviceNameIn));
    if (error == BLE_ERROR_NONE) {
        deviceName = deviceNameIn;
        if (operationMode == EDDYSTONE_MODE_CONFIG) {
            /* Need to update the advertising packets to the new name */
            setupEddystoneConfigScanResponse();
        }
    }

    return error;
}

/* It is not the responsibility of the Eddystone implementation to store
 * the configured parameters in persistent storage since this is
 * platform-specific. So we provide this function that returns the
 * configured values that need to be stored and the main application
 * takes care of storing them.
 */
void EddystoneService::getEddystoneParams(EddystoneParams_t &params)
{
    params.lockState      = lockState;
    params.flags          = flags;
    params.txPowerMode    = txPowerMode;
    params.urlFramePeriod = urlFramePeriod;
    params.tlmFramePeriod = tlmFramePeriod;
    params.uidFramePeriod = uidFramePeriod;
    params.tlmVersion     = tlmFrame.getTLMVersion();
    params.urlDataLength  = urlFrame.getEncodedURLDataLength();

    memcpy(params.advPowerLevels, advPowerLevels,               sizeof(PowerLevels_t));
    memcpy(params.lock,           lock,                         sizeof(Lock_t));
    memcpy(params.unlock,         unlock,                       sizeof(Lock_t));
    memcpy(params.urlData,        urlFrame.getEncodedURLData(), urlFrame.getEncodedURLDataLength());
    memcpy(params.uidNamespaceID, uidFrame.getUIDNamespaceID(), sizeof(UIDNamespaceID_t));
    memcpy(params.uidInstanceID,  uidFrame.getUIDInstanceID(),  sizeof(UIDInstanceID_t));
}

/* Helper function used only once during constructing the object to avoid
 * duplicated code.
 */
void EddystoneService::eddystoneConstructorHelper(const PowerLevels_t &advPowerLevelsIn,
                                                  const PowerLevels_t &radioPowerLevelsIn,
                                                  uint32_t            advConfigIntervalIn)
{
    /* We cannot use correctAdvertisementPeriod() for this check because the function
     * call to get the minimum advertising interval in the BLE API is different for
     * connectable and non-connectable advertising.
     */
    if (advConfigIntervalIn != 0) {
        if (advConfigIntervalIn < ble.gap().getMinAdvertisingInterval()) {
            advConfigInterval = ble.gap().getMinAdvertisingInterval();
        } else if (advConfigIntervalIn > ble.gap().getMaxAdvertisingInterval()) {
            advConfigInterval = ble.gap().getMaxAdvertisingInterval();
        } else {
            advConfigInterval = advConfigIntervalIn;
        }
    }

    memcpy(radioPowerLevels, radioPowerLevelsIn, sizeof(PowerLevels_t));
    memcpy(advPowerLevels,   advPowerLevelsIn,   sizeof(PowerLevels_t));

    /* TODO: Note that this timer is started from the time EddystoneService
     * is initialised and NOT from when the device is booted. So app needs
     * to take care that EddystoneService is one of the first things to be
     * started!
     */
    timeSinceBootTimer.start();

    /* Set the device name at startup */
    ble.gap().setDeviceName(reinterpret_cast<const uint8_t *>(deviceName));
}

/* When changing modes, we shutdown and init the BLE instance, so
 * this is needed to complete the initialisation task.
 */
void EddystoneService::bleInitComplete(BLE::InitializationCompleteCallbackContext* initContext)
{
    if (initContext->error != BLE_ERROR_NONE) {
        /* Initialisation failed */
        return;
    }

    switch (operationMode) {
    case EDDYSTONE_MODE_CONFIG:
        setupConfigService();
        break;
    case EDDYSTONE_MODE_BEACON:
        setupBeaconService();
        break;
    case EDDYSTONE_MODE_NONE:
        /* We don't need to do anything here, but it isn't an error */
        break;
    default:
        /* Some error occurred */
        error("Invalid EddystonService mode");
        break;
    }
}

void EddystoneService::swapAdvertisedFrame(FrameType frameType)
{
    switch(frameType) {
    case EDDYSTONE_FRAME_URL:
        updateAdvertisementPacket(rawUrlFrame, urlFrame.getRawFrameSize());
        break;
    case EDDYSTONE_FRAME_UID:
        updateAdvertisementPacket(rawUidFrame, uidFrame.getRawFrameSize());
        break;
    case EDDYSTONE_FRAME_TLM:
        updateRawTLMFrame();
        updateAdvertisementPacket(rawTlmFrame, tlmFrame.getRawFrameSize());
        break;
    default:
        /* Some error occurred */
        error("Frame to swap in does not specify a valid type");
        break;
    }
}

/* Helper function that calls user-defined functions to update Battery Voltage and Temperature (if available),
 * then updates the raw frame data and finally updates the actual advertised packet. This operation must be
 * done fairly often because the TLM frame TimeSinceBoot must have a 0.1 secs resolution according to the
 * Eddystone specification.
 */
void EddystoneService::updateRawTLMFrame(void)
{
    if (tlmBeaconTemperatureCallback != NULL) {
        tlmFrame.updateBeaconTemperature((*tlmBeaconTemperatureCallback)(tlmFrame.getBeaconTemperature()));
    }
    if (tlmBatteryVoltageCallback != NULL) {
        tlmFrame.updateBatteryVoltage((*tlmBatteryVoltageCallback)(tlmFrame.getBatteryVoltage()));
    }
    tlmFrame.updateTimeSinceBoot(timeSinceBootTimer.read_ms());
    tlmFrame.constructTLMFrame(rawTlmFrame);
}

void EddystoneService::updateAdvertisementPacket(const uint8_t* rawFrame, size_t rawFrameLength)
{
    ble.gap().clearAdvertisingPayload();
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_16BIT_SERVICE_IDS, EDDYSTONE_UUID, sizeof(EDDYSTONE_UUID));
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::SERVICE_DATA, rawFrame, rawFrameLength);
}

void EddystoneService::setupBeaconService(void)
{
    /* Initialise arrays to hold constructed raw frames */
    if (urlFramePeriod) {
        rawUrlFrame = new uint8_t[urlFrame.getRawFrameSize()];
        urlFrame.constructURLFrame(rawUrlFrame, advPowerLevels[txPowerMode]);
    }

    if (uidFramePeriod) {
        rawUidFrame = new uint8_t[uidFrame.getRawFrameSize()];
        uidFrame.constructUIDFrame(rawUidFrame, advPowerLevels[txPowerMode]);
    }

    if (tlmFramePeriod) {
        rawTlmFrame = new uint8_t[tlmFrame.getRawFrameSize()];
        /* Do not initialise because we have to reconstruct every 0.1 secs */
    }

    /* Configure advertisements */
    ble.gap().setTxPower(radioPowerLevels[txPowerMode]);
    ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_NON_CONNECTABLE_UNDIRECTED);
    ble.gap().setAdvertisingInterval(ble.gap().getMaxAdvertisingInterval());

    /* Make sure the queue is currently empty */
    advFrameQueue.reset();
    /* Setup callbacks to periodically add frames to be advertised to the queue and
     * add initial frame so that we have something to advertise on startup */
    if (uidFramePeriod) {
        advFrameQueue.push(EDDYSTONE_FRAME_UID);
        uidFrameCallbackHandle = eventQueue.call_every(
            uidFramePeriod,
            Callback<void(FrameType)>(this, &EddystoneService::enqueueFrame),
            EDDYSTONE_FRAME_UID
        );
    }
    if (tlmFramePeriod) {
        advFrameQueue.push(EDDYSTONE_FRAME_TLM);
        tlmFrameCallbackHandle = eventQueue.call_every(
            tlmFramePeriod,
            Callback<void(FrameType)>(this, &EddystoneService::enqueueFrame),
            EDDYSTONE_FRAME_TLM
        );
    }
    if (urlFramePeriod) {
        advFrameQueue.push(EDDYSTONE_FRAME_URL);
        tlmFrameCallbackHandle = eventQueue.call_every(
            urlFramePeriod,
            Callback<void(FrameType)>(this, &EddystoneService::enqueueFrame),
            EDDYSTONE_FRAME_URL
        );
    }

    /* Start advertising */
    manageRadio();
}

void EddystoneService::enqueueFrame(FrameType frameType)
{
    advFrameQueue.push(frameType);
    if (!radioManagerCallbackHandle) {
        /* Advertising stopped and there is not callback posted in the scheduler. Just
         * execute the manager to resume advertising */
        manageRadio();
    }
}

void EddystoneService::manageRadio(void)
{
    FrameType frameType;
    uint32_t  startTimeManageRadio = timeSinceBootTimer.read_ms();

    /* Signal that there is currently no callback posted */
    radioManagerCallbackHandle = 0;

    if (advFrameQueue.pop(frameType)) {
        /* We have something to advertise */
        if (ble.gap().getState().advertising) {
            ble.gap().stopAdvertising();
        }
        swapAdvertisedFrame(frameType);
        ble.gap().startAdvertising();

        /* Increase the advertised packet count in TLM frame */
        tlmFrame.updatePduCount();

        /* Post a callback to itself to stop the advertisement or pop the next
         * frame from the queue. However, take into account the time taken to
         * swap in this frame. */
        radioManagerCallbackHandle = eventQueue.call_in(
            ble.gap().getMinNonConnectableAdvertisingInterval() - (timeSinceBootTimer.read_ms() - startTimeManageRadio),
            Callback<void()>(this, &EddystoneService::manageRadio)
        );
    } else if (ble.gap().getState().advertising) {
        /* Nothing else to advertise, stop advertising and do not schedule any callbacks */
        ble.gap().stopAdvertising();
    }
}

void EddystoneService::setupConfigService(void)
{
    lockStateChar      = new ReadOnlyGattCharacteristic<bool>(UUID_LOCK_STATE_CHAR, &lockState);
    lockChar           = new WriteOnlyArrayGattCharacteristic<uint8_t, sizeof(Lock_t)>(UUID_LOCK_CHAR, lock);
    unlockChar         = new WriteOnlyArrayGattCharacteristic<uint8_t, sizeof(Lock_t)>(UUID_UNLOCK_CHAR, unlock);
    urlDataChar        = new GattCharacteristic(UUID_URL_DATA_CHAR, urlFrame.getEncodedURLData(), 0, URL_DATA_MAX, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE);
    flagsChar          = new ReadWriteGattCharacteristic<uint8_t>(UUID_FLAGS_CHAR, &flags);
    advPowerLevelsChar = new ReadWriteArrayGattCharacteristic<int8_t, sizeof(PowerLevels_t)>(UUID_ADV_POWER_LEVELS_CHAR, advPowerLevels);
    txPowerModeChar    = new ReadWriteGattCharacteristic<uint8_t>(UUID_TX_POWER_MODE_CHAR, &txPowerMode);
    beaconPeriodChar   = new ReadWriteGattCharacteristic<uint16_t>(UUID_BEACON_PERIOD_CHAR, &urlFramePeriod);
    resetChar          = new WriteOnlyGattCharacteristic<bool>(UUID_RESET_CHAR, &resetFlag);

    lockChar->setWriteAuthorizationCallback(this, &EddystoneService::lockAuthorizationCallback);
    unlockChar->setWriteAuthorizationCallback(this, &EddystoneService::unlockAuthorizationCallback);
    urlDataChar->setWriteAuthorizationCallback(this, &EddystoneService::urlDataWriteAuthorizationCallback);
    flagsChar->setWriteAuthorizationCallback(this, &EddystoneService::basicAuthorizationCallback<uint8_t>);
    advPowerLevelsChar->setWriteAuthorizationCallback(this, &EddystoneService::basicAuthorizationCallback<PowerLevels_t>);
    txPowerModeChar->setWriteAuthorizationCallback(this, &EddystoneService::powerModeAuthorizationCallback);
    beaconPeriodChar->setWriteAuthorizationCallback(this, &EddystoneService::basicAuthorizationCallback<uint16_t>);
    resetChar->setWriteAuthorizationCallback(this, &EddystoneService::basicAuthorizationCallback<bool>);

    charTable[0] = lockStateChar;
    charTable[1] = lockChar;
    charTable[2] = unlockChar;
    charTable[3] = urlDataChar;
    charTable[4] = flagsChar;
    charTable[5] = advPowerLevelsChar;
    charTable[6] = txPowerModeChar;
    charTable[7] = beaconPeriodChar;
    charTable[8] = resetChar;

    GattService configService(UUID_URL_BEACON_SERVICE, charTable, sizeof(charTable) / sizeof(GattCharacteristic *));

    ble.gattServer().addService(configService);
    ble.gattServer().onDataWritten(this, &EddystoneService::onDataWrittenCallback);
    updateCharacteristicValues();
    setupEddystoneConfigAdvertisements();
}

void EddystoneService::freeConfigCharacteristics(void)
{
    delete lockStateChar;
    delete lockChar;
    delete unlockChar;
    delete urlDataChar;
    delete flagsChar;
    delete advPowerLevelsChar;
    delete txPowerModeChar;
    delete beaconPeriodChar;
    delete resetChar;
}

void EddystoneService::stopBeaconService(void)
{
    /* Free unused memory */
    if (rawUrlFrame) {
        delete[] rawUrlFrame;
        rawUrlFrame = NULL;
    }
    if (rawUidFrame) {
        delete[] rawUidFrame;
        rawUidFrame = NULL;
    }
    if (rawTlmFrame) {
        delete[] rawTlmFrame;
        rawTlmFrame = NULL;
    }

    /* Unschedule callbacks */
    if (urlFrameCallbackHandle) {
        eventQueue.cancel(urlFrameCallbackHandle);
        urlFrameCallbackHandle = 0;
    }
    if (uidFrameCallbackHandle) {
        eventQueue.cancel(uidFrameCallbackHandle);
        uidFrameCallbackHandle = 0;
    }
    if (tlmFrameCallbackHandle) {
        eventQueue.cancel(tlmFrameCallbackHandle);
        tlmFrameCallbackHandle = 0;
    }
    if (radioManagerCallbackHandle) {
        eventQueue.cancel(radioManagerCallbackHandle);
        radioManagerCallbackHandle = 0;
    }
}

/*
 * Internal helper function used to update the GATT database following any
 * change to the internal state of the service object.
 */
void EddystoneService::updateCharacteristicValues(void)
{
    ble.gattServer().write(lockStateChar->getValueHandle(), reinterpret_cast<uint8_t *>(&lockState), sizeof(bool));
    ble.gattServer().write(urlDataChar->getValueHandle(), urlFrame.getEncodedURLData(), urlFrame.getEncodedURLDataLength());
    ble.gattServer().write(flagsChar->getValueHandle(), &flags, sizeof(uint8_t));
    ble.gattServer().write(beaconPeriodChar->getValueHandle(), reinterpret_cast<uint8_t *>(&urlFramePeriod), sizeof(uint16_t));
    ble.gattServer().write(txPowerModeChar->getValueHandle(), &txPowerMode, sizeof(uint8_t));
    ble.gattServer().write(advPowerLevelsChar->getValueHandle(), reinterpret_cast<uint8_t *>(advPowerLevels), sizeof(PowerLevels_t));
    ble.gattServer().write(lockChar->getValueHandle(), lock, sizeof(PowerLevels_t));
    ble.gattServer().write(unlockChar->getValueHandle(), unlock, sizeof(PowerLevels_t));
}

void EddystoneService::setupEddystoneConfigAdvertisements(void)
{
    ble.gap().clearAdvertisingPayload();

    /* Accumulate the new payload */
    ble.gap().accumulateAdvertisingPayload(
        GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE
    );
    /* UUID is in different order in the ADV frame (!) */
    uint8_t reversedServiceUUID[sizeof(UUID_URL_BEACON_SERVICE)];
    for (size_t i = 0; i < sizeof(UUID_URL_BEACON_SERVICE); i++) {
        reversedServiceUUID[i] = UUID_URL_BEACON_SERVICE[sizeof(UUID_URL_BEACON_SERVICE) - i - 1];
    }
    ble.gap().accumulateAdvertisingPayload(
        GapAdvertisingData::COMPLETE_LIST_128BIT_SERVICE_IDS,
        reversedServiceUUID,
        sizeof(reversedServiceUUID)
    );
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::GENERIC_TAG);
    setupEddystoneConfigScanResponse();

    ble.gap().setTxPower(radioPowerLevels[txPowerMode]);
    ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    ble.gap().setAdvertisingInterval(advConfigInterval);
    ble.gap().startAdvertising();
}

void EddystoneService::setupEddystoneConfigScanResponse(void)
{
    ble.gap().clearScanResponse();
    ble.gap().accumulateScanResponse(
        GapAdvertisingData::COMPLETE_LOCAL_NAME,
        reinterpret_cast<const uint8_t *>(deviceName),
        strlen(deviceName)
    );
    ble.gap().accumulateScanResponse(
        GapAdvertisingData::TX_POWER_LEVEL,
        reinterpret_cast<uint8_t *>(&advPowerLevels[TX_POWER_MODE_LOW]),
        sizeof(uint8_t)
    );
}

void EddystoneService::lockAuthorizationCallback(GattWriteAuthCallbackParams *authParams)
{
    if (lockState) {
        authParams->authorizationReply = AUTH_CALLBACK_REPLY_ATTERR_INSUF_AUTHORIZATION;
    } else if (authParams->len != sizeof(Lock_t)) {
        authParams->authorizationReply = AUTH_CALLBACK_REPLY_ATTERR_INVALID_ATT_VAL_LENGTH;
    } else if (authParams->offset != 0) {
        authParams->authorizationReply = AUTH_CALLBACK_REPLY_ATTERR_INVALID_OFFSET;
    } else {
        authParams->authorizationReply = AUTH_CALLBACK_REPLY_SUCCESS;
    }
}

void EddystoneService::unlockAuthorizationCallback(GattWriteAuthCallbackParams *authParams)
{
    if (!lockState && (authParams->len == sizeof(Lock_t))) {
        authParams->authorizationReply = AUTH_CALLBACK_REPLY_SUCCESS;
    } else if (authParams->len != sizeof(Lock_t)) {
        authParams->authorizationReply = AUTH_CALLBACK_REPLY_ATTERR_INVALID_ATT_VAL_LENGTH;
    } else if (authParams->offset != 0) {
        authParams->authorizationReply = AUTH_CALLBACK_REPLY_ATTERR_INVALID_OFFSET;
    } else if (memcmp(authParams->data, lock, sizeof(Lock_t)) != 0) {
        authParams->authorizationReply = AUTH_CALLBACK_REPLY_ATTERR_INSUF_AUTHORIZATION;
    } else {
        authParams->authorizationReply = AUTH_CALLBACK_REPLY_SUCCESS;
    }
}

void EddystoneService::urlDataWriteAuthorizationCallback(GattWriteAuthCallbackParams *authParams)
{
    if (lockState) {
        authParams->authorizationReply = AUTH_CALLBACK_REPLY_ATTERR_INSUF_AUTHORIZATION;
    } else if (authParams->offset != 0) {
        authParams->authorizationReply = AUTH_CALLBACK_REPLY_ATTERR_INVALID_OFFSET;
    } else {
        authParams->authorizationReply = AUTH_CALLBACK_REPLY_SUCCESS;
    }
}

void EddystoneService::powerModeAuthorizationCallback(GattWriteAuthCallbackParams *authParams)
{
    if (lockState) {
        authParams->authorizationReply = AUTH_CALLBACK_REPLY_ATTERR_INSUF_AUTHORIZATION;
    } else if (authParams->len != sizeof(uint8_t)) {
        authParams->authorizationReply = AUTH_CALLBACK_REPLY_ATTERR_INVALID_ATT_VAL_LENGTH;
    } else if (authParams->offset != 0) {
        authParams->authorizationReply = AUTH_CALLBACK_REPLY_ATTERR_INVALID_OFFSET;
    } else if (*((uint8_t *)authParams->data) >= NUM_POWER_MODES) {
        authParams->authorizationReply = AUTH_CALLBACK_REPLY_ATTERR_WRITE_NOT_PERMITTED;
    } else {
        authParams->authorizationReply = AUTH_CALLBACK_REPLY_SUCCESS;
    }
}

template <typename T>
void EddystoneService::basicAuthorizationCallback(GattWriteAuthCallbackParams *authParams)
{
    if (lockState) {
        authParams->authorizationReply = AUTH_CALLBACK_REPLY_ATTERR_INSUF_AUTHORIZATION;
    } else if (authParams->len != sizeof(T)) {
        authParams->authorizationReply = AUTH_CALLBACK_REPLY_ATTERR_INVALID_ATT_VAL_LENGTH;
    } else if (authParams->offset != 0) {
        authParams->authorizationReply = AUTH_CALLBACK_REPLY_ATTERR_INVALID_OFFSET;
    } else {
        authParams->authorizationReply = AUTH_CALLBACK_REPLY_SUCCESS;
    }
}

/*
 * This callback is invoked when a GATT client attempts to modify any of the
 * characteristics of this service. Attempts to do so are also applied to
 * the internal state of this service object.
 */
void EddystoneService::onDataWrittenCallback(const GattWriteCallbackParams *writeParams)
{
    uint16_t handle = writeParams->handle;

    if (handle == lockChar->getValueHandle()) {
        memcpy(lock, writeParams->data, sizeof(Lock_t));
        /* Set the state to be locked by the lock code (note: zeros are a valid lock) */
        lockState = true;
        ble.gattServer().write(lockChar->getValueHandle(), lock, sizeof(PowerLevels_t));
        ble.gattServer().write(lockStateChar->getValueHandle(), reinterpret_cast<uint8_t *>(&lockState), sizeof(bool));
    } else if (handle == unlockChar->getValueHandle()) {
        /* Validated earlier */
        lockState = false;
        ble.gattServer().write(unlockChar->getValueHandle(), unlock, sizeof(PowerLevels_t));
        ble.gattServer().write(lockStateChar->getValueHandle(), reinterpret_cast<uint8_t *>(&lockState), sizeof(bool));
    } else if (handle == urlDataChar->getValueHandle()) {
        urlFrame.setEncodedURLData(writeParams->data, writeParams->len);
        ble.gattServer().write(urlDataChar->getValueHandle(), urlFrame.getEncodedURLData(), urlFrame.getEncodedURLDataLength());
    } else if (handle == flagsChar->getValueHandle()) {
        flags = *(writeParams->data);
        ble.gattServer().write(flagsChar->getValueHandle(), &flags, sizeof(uint8_t));
    } else if (handle == advPowerLevelsChar->getValueHandle()) {
        memcpy(advPowerLevels, writeParams->data, sizeof(PowerLevels_t));
        ble.gattServer().write(advPowerLevelsChar->getValueHandle(), reinterpret_cast<uint8_t *>(advPowerLevels), sizeof(PowerLevels_t));
    } else if (handle == txPowerModeChar->getValueHandle()) {
        txPowerMode = *(writeParams->data);
        ble.gattServer().write(txPowerModeChar->getValueHandle(), &txPowerMode, sizeof(uint8_t));
    } else if (handle == beaconPeriodChar->getValueHandle()) {
        uint16_t tmpBeaconPeriod = correctAdvertisementPeriod(*((uint16_t *)(writeParams->data)));
        if (tmpBeaconPeriod != urlFramePeriod) {
            urlFramePeriod = tmpBeaconPeriod;
            ble.gattServer().write(beaconPeriodChar->getValueHandle(), reinterpret_cast<uint8_t *>(&urlFramePeriod), sizeof(uint16_t));
        }
    } else if (handle == resetChar->getValueHandle() && (*((uint8_t *)writeParams->data) != 0)) {
        /* Reset characteristics to default values */
        flags          = 0;
        txPowerMode    = TX_POWER_MODE_LOW;
        urlFramePeriod = DEFAULT_URL_FRAME_PERIOD_MSEC;

        urlFrame.setURLData(DEFAULT_URL);
        memset(lock, 0, sizeof(Lock_t));

        ble.gattServer().write(urlDataChar->getValueHandle(), urlFrame.getEncodedURLData(), urlFrame.getEncodedURLDataLength());
        ble.gattServer().write(flagsChar->getValueHandle(), &flags, sizeof(uint8_t));
        ble.gattServer().write(txPowerModeChar->getValueHandle(), &txPowerMode, sizeof(uint8_t));
        ble.gattServer().write(beaconPeriodChar->getValueHandle(), reinterpret_cast<uint8_t *>(&urlFramePeriod), sizeof(uint16_t));
        ble.gattServer().write(lockChar->getValueHandle(), lock, sizeof(PowerLevels_t));
    }
}

uint16_t EddystoneService::correctAdvertisementPeriod(uint16_t beaconPeriodIn) const
{
    /* Re-map beaconPeriod to within permissible bounds if necessary. */
    if (beaconPeriodIn != 0) {
        if (beaconPeriodIn < ble.gap().getMinNonConnectableAdvertisingInterval()) {
            return ble.gap().getMinNonConnectableAdvertisingInterval();
        } else if (beaconPeriodIn > ble.gap().getMaxAdvertisingInterval()) {
            return ble.gap().getMaxAdvertisingInterval();
        }
    }
    return beaconPeriodIn;
}

void EddystoneService::setURLFrameAdvertisingInterval(uint16_t urlFrameIntervalIn)
{
    if (urlFrameIntervalIn == urlFramePeriod) {
        /* Do nothing */
        return;
    }

    /* Make sure the input period is within bounds */
    urlFramePeriod = correctAdvertisementPeriod(urlFrameIntervalIn);

    if (operationMode == EDDYSTONE_MODE_BEACON) {
        if (urlFrameCallbackHandle) {
            eventQueue.cancel(urlFrameCallbackHandle);
        } else {
            /* This frame was just enabled */
            if (!rawUidFrame && urlFramePeriod) {
                /* Allocate memory for this frame and construct it */
                rawUrlFrame = new uint8_t[urlFrame.getRawFrameSize()];
                urlFrame.constructURLFrame(rawUrlFrame, advPowerLevels[txPowerMode]);
            }
        }

        if (urlFramePeriod) {
            /* Currently the only way to change the period of a callback
             * is to cancel it and reschedule
             */
            urlFrameCallbackHandle = eventQueue.call_every(
                urlFramePeriod,
                Callback<void(FrameType)>(this, &EddystoneService::enqueueFrame),
                EDDYSTONE_FRAME_URL
            );
        } else {
            urlFrameCallbackHandle = 0;
        }
    } else if (operationMode == EDDYSTONE_MODE_CONFIG) {
        ble.gattServer().write(beaconPeriodChar->getValueHandle(), reinterpret_cast<uint8_t *>(&urlFramePeriod), sizeof(uint16_t));
    }
}

void EddystoneService::setUIDFrameAdvertisingInterval(uint16_t uidFrameIntervalIn)
{
    if (uidFrameIntervalIn == uidFramePeriod) {
        /* Do nothing */
        return;
    }

    /* Make sure the input period is within bounds */
    uidFramePeriod = correctAdvertisementPeriod(uidFrameIntervalIn);

    if (operationMode == EDDYSTONE_MODE_BEACON) {
        if (uidFrameCallbackHandle) {
            /* The advertisement interval changes, update the periodic callback */
            eventQueue.cancel(uidFrameCallbackHandle);
        } else {
            /* This frame was just enabled */
            if (!rawUidFrame && uidFramePeriod) {
                /* Allocate memory for this frame and construct it */
                rawUidFrame = new uint8_t[uidFrame.getRawFrameSize()];
                uidFrame.constructUIDFrame(rawUidFrame, advPowerLevels[txPowerMode]);
            }
        }

        if (uidFramePeriod) {
            /* Currently the only way to change the period of a callback
             * is to cancel it and reschedule
             */
            uidFrameCallbackHandle = eventQueue.call_every(
                uidFramePeriod,
                Callback<void(FrameType)>(this, &EddystoneService::enqueueFrame),
                EDDYSTONE_FRAME_UID
            );
        } else {
            uidFrameCallbackHandle = 0;
        }
    }
}

void EddystoneService::setTLMFrameAdvertisingInterval(uint16_t tlmFrameIntervalIn)
{
    if (tlmFrameIntervalIn == tlmFramePeriod) {
        /* Do nothing */
        return;
    }

    /* Make sure the input period is within bounds */
    tlmFramePeriod = correctAdvertisementPeriod(tlmFrameIntervalIn);

    if (operationMode == EDDYSTONE_MODE_BEACON) {
        if (tlmFrameCallbackHandle) {
            /* The advertisement interval changes, update periodic callback */
            eventQueue.cancel(tlmFrameCallbackHandle);
        } else {
            /* This frame was just enabled */
            if (!rawTlmFrame && tlmFramePeriod) {
                /* Allocate memory for this frame and construct it */
                rawTlmFrame = new uint8_t[tlmFrame.getRawFrameSize()];
                /* Do not construct the TLM frame because this changes every 0.1 seconds */
            }
        }

        if (tlmFramePeriod) {
            /* Currently the only way to change the period of a callback
             * is to cancel it and reschedule
             */
            tlmFrameCallbackHandle = eventQueue.call_every(
                tlmFramePeriod,
                Callback<void(FrameType)>(this, &EddystoneService::enqueueFrame),
                EDDYSTONE_FRAME_TLM
            );
        } else {
            tlmFrameCallbackHandle = 0;
        }
    }
}
