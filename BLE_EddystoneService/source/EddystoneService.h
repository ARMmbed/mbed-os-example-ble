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

#ifndef __EDDYSTONESERVICE_H__
#define __EDDYSTONESERVICE_H__

#include "ble/BLE.h"
#ifdef YOTTA_CFG_MBED_OS
    #include "mbed-drivers/mbed.h"
#else
    #include "mbed.h"
#endif

#define UUID_URL_BEACON(FIRST, SECOND) {                         \
        0xee, 0x0c, FIRST, SECOND, 0x87, 0x86, 0x40, 0xba,       \
        0xab, 0x96, 0x99, 0xb9, 0x1a, 0xc9, 0x81, 0xd8,          \
}

static const uint8_t EDDYSTONE_UUID[] = {0xAA, 0xFE};

static const uint8_t UUID_URL_BEACON_SERVICE[]    = UUID_URL_BEACON(0x20, 0x80);
static const uint8_t UUID_LOCK_STATE_CHAR[]       = UUID_URL_BEACON(0x20, 0x81);
static const uint8_t UUID_LOCK_CHAR[]             = UUID_URL_BEACON(0x20, 0x82);
static const uint8_t UUID_UNLOCK_CHAR[]           = UUID_URL_BEACON(0x20, 0x83);
static const uint8_t UUID_URL_DATA_CHAR[]         = UUID_URL_BEACON(0x20, 0x84);
static const uint8_t UUID_FLAGS_CHAR[]            = UUID_URL_BEACON(0x20, 0x85);
static const uint8_t UUID_ADV_POWER_LEVELS_CHAR[] = UUID_URL_BEACON(0x20, 0x86);
static const uint8_t UUID_TX_POWER_MODE_CHAR[]    = UUID_URL_BEACON(0x20, 0x87);
static const uint8_t UUID_BEACON_PERIOD_CHAR[]    = UUID_URL_BEACON(0x20, 0x88);
static const uint8_t UUID_RESET_CHAR[]            = UUID_URL_BEACON(0x20, 0x89);

const char DEVICE_NAME[] = "EDDYSTONE CONFIG";

const char DEFAULT_URL[] = "http://www.mbed.com/";

class EddystoneService
{
public:
    enum {
        TX_POWER_MODE_LOWEST,
        TX_POWER_MODE_LOW,
        TX_POWER_MODE_MEDIUM,
        TX_POWER_MODE_HIGH,
        NUM_POWER_MODES
    };

    /* Operation modes of the EddystoneService:
     *      NONE: EddystoneService has been initialised but no memory has been
     *            dynamically allocated. Additionally, no services are running
     *            nothing is being advertised.
     *      CONFIG: EddystoneService has been initialised, the configuration
     *              service started and memory has been allocated for BLE
     *              characteristics. Memory consumption peaks during CONFIG
     *              mode.
     *      BEACON: Eddystone service is running as a beacon advertising URL,
     *              UID and/or TLM frames depending on how it is configured.
     * Note: The main app can change the mode of EddystoneService at any point
     * of time by calling startConfigService() or startBeaconService().
     * Resources from the previous mode will be freed. It is currently NOT
     * possible to force EddystoneService back into MODE_NONE.
     */
    enum OperationModes {
        EDDYSTONE_MODE_NONE,
        EDDYSTONE_MODE_CONFIG,
        EDDYSTONE_MODE_BEACON
    };

    enum FrameType {
        EDDYSTONE_FRAME_URL,
        EDDYSTONE_FRAME_UID,
        EDDYSTONE_FRAME_TLM,
        NUM_EDDYSTONE_FRAMES
    };

    enum EddystoneError_t {
        EDDYSTONE_ERROR_NONE,
        EDDYSTONE_ERROR_INVALID_BEACON_PERIOD,
        EDDYSTONE_ERROR_INVALID_CONSEC_FRAMES,
        EDDYSTONE_ERROR_INVALID_ADVERTISING_INTERVAL
    };

    static const uint32_t DEFAULT_CONFIG_PERIOD_MSEC = 1000;

    static const uint16_t DEFAULT_BEACON_PERIOD_MSEC = 1000;

    /* 128 bits of lock */
    typedef uint8_t Lock_t[16];
    typedef int8_t PowerLevels_t[NUM_POWER_MODES];

    static const uint16_t URL_DATA_MAX = 18;
    typedef uint8_t UrlData_t[URL_DATA_MAX];

    /* UID Frame Type subfields */
    static const size_t UID_NAMESPACEID_SIZE = 10;
    typedef uint8_t UIDNamespaceID_t[UID_NAMESPACEID_SIZE];
    static const size_t UID_INSTANCEID_SIZE = 6;
    typedef uint8_t UIDInstanceID_t[UID_INSTANCEID_SIZE];

    /* Callbacks for updating BateryVoltage and Temperature */
    typedef uint16_t (*TlmUpdateCallback_t) (uint16_t);

    /* Size of Eddystone UUID needed for all frames */
    static const uint16_t EDDYSTONE_UUID_SIZE = sizeof(EDDYSTONE_UUID);

    static const uint16_t TOTAL_CHARACTERISTICS = 9;

    struct EddystoneParams_t {
        bool             lockState;
        Lock_t           lock;
        Lock_t           unlock;
        uint8_t          flags;
        PowerLevels_t    advPowerLevels;
        uint8_t          txPowerMode;
        uint16_t         beaconPeriod;
        uint8_t          tlmVersion;
        uint8_t          urlDataLength;
        UrlData_t        urlData;
        UIDNamespaceID_t uidNamespaceID;
        UIDInstanceID_t  uidInstanceID;
    };

    /* Initialise the EddystoneService using parameters from persistent storage */
    EddystoneService(BLE                 &bleIn,
                     EddystoneParams_t   &paramsIn,
                     const PowerLevels_t &advPowerLevelsIn,
                     const PowerLevels_t &radioPowerLevelsIn,
                     uint32_t            advConfigIntervalIn = DEFAULT_CONFIG_PERIOD_MSEC) :
        ble(bleIn),
        operationMode(EDDYSTONE_MODE_NONE),
        urlFrame(paramsIn.urlData, paramsIn.urlDataLength),
        uidFrame(paramsIn.uidNamespaceID, paramsIn.uidInstanceID),
        tlmFrame(paramsIn.tlmVersion),
        resetFlag(false),
        tlmBatteryVoltageCallback(NULL),
        tlmBeaconTemperatureCallback(NULL)
    {
        lockState         = paramsIn.lockState;
        flags             = paramsIn.flags;
        txPowerMode       = paramsIn.txPowerMode;
        beaconPeriod      = correctAdvertisementPeriod(paramsIn.beaconPeriod);

        memcpy(lock,             paramsIn.lock,      sizeof(Lock_t));
        memcpy(unlock,           paramsIn.unlock,    sizeof(Lock_t));

        eddystoneConstructorHelper(advPowerLevelsIn, radioPowerLevelsIn, advConfigIntervalIn);
    }

    /* When using this constructor we need to call setURLData,
     * setTMLData and setUIDData to initialise values manually
     */
    EddystoneService(BLE                 &bleIn,
                     const PowerLevels_t &advPowerLevelsIn,
                     const PowerLevels_t &radioPowerLevelsIn,
                     uint32_t            advConfigIntervalIn = DEFAULT_CONFIG_PERIOD_MSEC) :
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
        beaconPeriod(DEFAULT_BEACON_PERIOD_MSEC),
        tlmBatteryVoltageCallback(NULL),
        tlmBeaconTemperatureCallback(NULL)
    {
        eddystoneConstructorHelper(advPowerLevelsIn, radioPowerLevelsIn, advConfigIntervalIn);
    }

    /* Setup callback to update BatteryVoltage in TLM frame */
    void onTLMBatteryVoltageUpdate(TlmUpdateCallback_t tlmBatteryVoltageCallbackIn)
    {
        tlmBatteryVoltageCallback = tlmBatteryVoltageCallbackIn;
    }

    /* Setup callback to update BeaconTemperature in TLM frame */
    void onTLMBeaconTemperatureUpdate(TlmUpdateCallback_t tlmBeaconTemperatureCallbackIn)
    {
        tlmBeaconTemperatureCallback = tlmBeaconTemperatureCallbackIn;
    }

    void setTLMData(uint8_t tlmVersionIn = 0)
    {
       tlmFrame.setTLMData(tlmVersionIn);
    }

    void setURLData(const char *urlDataIn)
    {
        urlFrame.setURLData(urlDataIn);
    }

    void setUIDData(const UIDNamespaceID_t *uidNamespaceIDIn, const UIDInstanceID_t *uidInstanceIDIn)
    {
        uidFrame.setUIDData(uidNamespaceIDIn, uidInstanceIDIn);
    }

    EddystoneError_t startConfigService(void)
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
            /* Free unused memory */
            freeBeaconFrames();
            operationMode = EDDYSTONE_MODE_CONFIG;
            ble.init(this, &EddystoneService::bleInitComplete);
            return EDDYSTONE_ERROR_NONE;
        }

        operationMode = EDDYSTONE_MODE_CONFIG;
        setupConfigService();
        return EDDYSTONE_ERROR_NONE;
    }

    EddystoneError_t startBeaconService(uint16_t consecUrlFramesIn = 2, uint16_t consecUidFramesIn = 2, uint16_t consecTlmFramesIn = 2)
    {
        if (operationMode == EDDYSTONE_MODE_BEACON) {
            /* Nothing to do, we are already in beacon mode */
            return EDDYSTONE_ERROR_NONE;
        } else if (!consecUrlFramesIn && !consecUidFramesIn && !consecTlmFramesIn) {
            /* Nothing to do, the user wants 0 consecutive frames of everything */
            return EDDYSTONE_ERROR_INVALID_CONSEC_FRAMES;
        } else if (!beaconPeriod) {
            /* Nothing to do, the period is 0 for all frames */
            return EDDYSTONE_ERROR_INVALID_BEACON_PERIOD;
        }

        /* Setup tracking of the current advertised frame. Note that this will
         * cause URL or UID frames to be advertised first!
         */
        currentAdvertisedFrame            = EDDYSTONE_FRAME_TLM;
        consecFrames[EDDYSTONE_FRAME_URL] = consecUrlFramesIn;
        consecFrames[EDDYSTONE_FRAME_UID] = consecUidFramesIn;
        consecFrames[EDDYSTONE_FRAME_TLM] = consecTlmFramesIn;

        memset(currentConsecFrames, 0, sizeof(uint16_t) * NUM_EDDYSTONE_FRAMES);

        if (operationMode == EDDYSTONE_MODE_CONFIG) {
            ble.shutdown();
            /* Free unused memory */
            freeConfigCharacteristics();
            operationMode = EDDYSTONE_MODE_BEACON;
            ble.init(this, &EddystoneService::bleInitComplete);
            return EDDYSTONE_ERROR_NONE;
        }

        operationMode = EDDYSTONE_MODE_BEACON;
        setupBeaconService();
        return EDDYSTONE_ERROR_NONE;
    }

    /* It is not the responsibility of the Eddystone implementation to store
     * the configured parameters in persistent storage since this is
     * platform-specific. So we provide this function that returns the
     * configured values that need to be stored and the main application
     * takes care of storing them.
     */
    void getEddystoneParams(EddystoneParams_t *params)
    {
        params->lockState     = lockState;
        params->flags         = flags;
        params->txPowerMode   = txPowerMode;
        params->beaconPeriod  = beaconPeriod;
        params->tlmVersion    = tlmFrame.getTLMVersion();
        params->urlDataLength = urlFrame.getEncodedURLDataLength();

        memcpy(params->advPowerLevels, advPowerLevels,               sizeof(PowerLevels_t));
        memcpy(params->lock,           lock,                         sizeof(Lock_t));
        memcpy(params->unlock,         unlock,                       sizeof(Lock_t));
        memcpy(params->urlData,        urlFrame.getEncodedURLData(), urlFrame.getEncodedURLDataLength());
        memcpy(params->uidNamespaceID, uidFrame.getUIDNamespaceID(), sizeof(UIDNamespaceID_t));
        memcpy(params->uidInstanceID,  uidFrame.getUIDInstanceID(),  sizeof(UIDInstanceID_t));
    }

private:

    /* Helper function used only once during constructing the object to avoid
     * duplicated code.
     */
    void eddystoneConstructorHelper(const PowerLevels_t &advPowerLevelsIn,
                                    const PowerLevels_t &radioPowerLevelsIn,
                                    uint32_t            advConfigIntervalIn)
    {
        advConfigInterval = (advConfigIntervalIn > 0) ? correctAdvertisementPeriod(advConfigIntervalIn) : 0;

        memcpy(radioPowerLevels, radioPowerLevelsIn, sizeof(PowerLevels_t));
        memcpy(advPowerLevels,   advPowerLevelsIn,   sizeof(PowerLevels_t));

        /* TODO: Note that this timer is started from the time EddystoneService
         * is initialised and NOT from when the device is booted. So app needs
         * to take care that EddystoneService is one of the first things to be
         * started!
         */
        timeSinceBootTimer.start();
    }

    /* When changing modes, we shutdown and init the BLE instance, so
     * this is needed to complete the initialisation task.
     */
    void bleInitComplete(BLE::InitializationCompleteCallbackContext* initContext)
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
        default:
            /* Some error occurred */
            break;
        }
    }

    void swapAdvertisedFrame(void)
    {
        /* This essentially works out which is the next frame to be swapped in
         * and updated the advertised packets. It will eventually terminate
         * and in the worst case the frame swapped in is the current advertised
         * frame.
         */
        while (true) {
            currentAdvertisedFrame = (currentAdvertisedFrame + 1) % NUM_EDDYSTONE_FRAMES;

            if (currentAdvertisedFrame == EDDYSTONE_FRAME_URL && consecFrames[EDDYSTONE_FRAME_URL] > 0) {
                updateAdvertisementPacket(rawUrlFrame, urlFrame.getRawFrameSize());
                return;
            } else if (currentAdvertisedFrame == EDDYSTONE_FRAME_UID && consecFrames[EDDYSTONE_FRAME_UID] > 0) {
                updateAdvertisementPacket(rawUidFrame, uidFrame.getRawFrameSize());
                return;
            } else if (currentAdvertisedFrame == EDDYSTONE_FRAME_TLM && consecFrames[EDDYSTONE_FRAME_UID] > 0) {
                updateRawTLMFrame();
                updateAdvertisementPacket(rawTlmFrame, tlmFrame.getRawFrameSize());
                return;
            }
        }
    }

    /* Helper function that calls user-defined functions to update Battery Voltage and Temperature (if available),
     * then updates the raw frame data and finally updates the actual advertised packet. This operation must be
     * done fairly often because the TLM frame TimeSinceBoot must have a 0.1 secs resolution according to the
     * Eddystone specification.
     */
    void updateRawTLMFrame(void)
    {
        if (tlmBeaconTemperatureCallback != NULL) {
            tlmFrame.updateBeaconTemperature((*tlmBeaconTemperatureCallback)(tlmFrame.getBeaconTemperature()));
        }
        if (tlmBatteryVoltageCallback != NULL) {
            tlmFrame.updateBatteryVoltage((*tlmBatteryVoltageCallback)(tlmFrame.getBatteryVoltage()));
        }
        tlmFrame.updateTimeSinceBoot(timeSinceBootTimer.read_ms());
        tlmFrame.constructTLMFrame(rawTlmFrame, *this);
    }

    void updateAdvertisementPacket(const uint8_t* rawFrame, size_t rawFrameLength) {
        ble.gap().clearAdvertisingPayload();
        ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
        ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_16BIT_SERVICE_IDS, EDDYSTONE_UUID, sizeof(EDDYSTONE_UUID));
        ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::SERVICE_DATA, rawFrame, rawFrameLength);
    }

    void setupBeaconService(void)
    {
        /* Initialise arrays to hold constructed raw frames */
        if (consecFrames[EDDYSTONE_FRAME_URL] > 0) {
            rawUrlFrame = new uint8_t[urlFrame.getRawFrameSize()];
            urlFrame.constructURLFrame(rawUrlFrame, *this);
        }

        if (consecFrames[EDDYSTONE_FRAME_UID] > 0) {
            rawUidFrame = new uint8_t[uidFrame.getRawFrameSize()];
            uidFrame.constructUIDFrame(rawUidFrame, *this);
        }

        if (consecFrames[EDDYSTONE_FRAME_TLM] > 0) {
            rawTlmFrame = new uint8_t[tlmFrame.getRawFrameSize()];
            /* Do not initialise because we have to reconstruct every 0.1 secs */
        }

        /* Configure advertisements */
        ble.gap().setTxPower(radioPowerLevels[txPowerMode]);
        ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_NON_CONNECTABLE_UNDIRECTED);
        ble.gap().setAdvertisingInterval(beaconPeriod);
        ble.gap().onRadioNotification(this, &EddystoneService::radioNotificationCallback);

        /* Set advertisement packet payload */
        swapAdvertisedFrame();

        /* Start advertising */
        ble.gap().startAdvertising();
    }

    void setupConfigService(void)
    {
        lockStateChar      = new ReadOnlyGattCharacteristic<bool>(UUID_LOCK_STATE_CHAR, &lockState);
        lockChar           = new WriteOnlyArrayGattCharacteristic<uint8_t, sizeof(Lock_t)>(UUID_LOCK_CHAR, lock);
        unlockChar         = new WriteOnlyArrayGattCharacteristic<uint8_t, sizeof(Lock_t)>(UUID_UNLOCK_CHAR, unlock);
        urlDataChar        = new GattCharacteristic(UUID_URL_DATA_CHAR, urlFrame.getEncodedURLData(), 0, URL_DATA_MAX, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ | GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE);
        flagsChar          = new ReadWriteGattCharacteristic<uint8_t>(UUID_FLAGS_CHAR, &flags);
        advPowerLevelsChar = new ReadWriteArrayGattCharacteristic<int8_t, sizeof(PowerLevels_t)>(UUID_ADV_POWER_LEVELS_CHAR, advPowerLevels);
        txPowerModeChar    = new ReadWriteGattCharacteristic<uint8_t>(UUID_TX_POWER_MODE_CHAR, &txPowerMode);
        beaconPeriodChar   = new ReadWriteGattCharacteristic<uint16_t>(UUID_BEACON_PERIOD_CHAR, &beaconPeriod);
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

    void freeConfigCharacteristics(void)
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

    void freeBeaconFrames(void)
    {
        delete[] rawUrlFrame;
        delete[] rawUidFrame;
        delete[] rawTlmFrame;
    }

    void radioNotificationCallback(bool radioActive)
    {
        if (radioActive) {
            /* Do nothing */
            return;
        }

        tlmFrame.updatePduCount();
        currentConsecFrames[currentAdvertisedFrame]++;

        if (consecFrames[currentAdvertisedFrame] > currentConsecFrames[currentAdvertisedFrame]) {
            if (currentAdvertisedFrame == EDDYSTONE_FRAME_TLM) {
                /* Update the TLM frame otherwise we will not meet the 0.1 secs resolution of
                 * the Eddystone specification.
                 */
                updateRawTLMFrame();
                updateAdvertisementPacket(rawTlmFrame, tlmFrame.getRawFrameSize());
            }
            /* Keep advertising the same frame */
            return;
        }

        currentConsecFrames[currentAdvertisedFrame] = 0;

#ifdef YOTTA_CFG_MBED_OS
        minar::Scheduler::postCallback(this, &EddystoneService::swapAdvertisedFrame);
#else
        swapAdvertisedFrameTimeout.attach_us(this, &EddystoneService::swapAdvertisedFrame);
#endif
    }

    /*
     * Internal helper function used to update the GATT database following any
     * change to the internal state of the service object.
     */
    void updateCharacteristicValues(void)
    {
        ble.gattServer().write(lockStateChar->getValueHandle(), reinterpret_cast<uint8_t *>(&lockState), sizeof(bool));
        ble.gattServer().write(urlDataChar->getValueHandle(), urlFrame.getEncodedURLData(), urlFrame.getEncodedURLDataLength());
        ble.gattServer().write(flagsChar->getValueHandle(), &flags, sizeof(uint8_t));
        ble.gattServer().write(beaconPeriodChar->getValueHandle(), reinterpret_cast<uint8_t *>(&beaconPeriod), sizeof(uint16_t));
        ble.gattServer().write(txPowerModeChar->getValueHandle(), &txPowerMode, sizeof(uint8_t));
        ble.gattServer().write(advPowerLevelsChar->getValueHandle(), reinterpret_cast<uint8_t *>(advPowerLevels), sizeof(PowerLevels_t));
        ble.gattServer().write(lockChar->getValueHandle(), lock, sizeof(PowerLevels_t));
        ble.gattServer().write(unlockChar->getValueHandle(), unlock, sizeof(PowerLevels_t));
    }

    void setupEddystoneConfigAdvertisements(void)
    {
        ble.gap().clearAdvertisingPayload();
        ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);

        /* UUID is in different order in the ADV frame (!) */
        uint8_t reversedServiceUUID[sizeof(UUID_URL_BEACON_SERVICE)];
        for (size_t i = 0; i < sizeof(UUID_URL_BEACON_SERVICE); i++) {
            reversedServiceUUID[i] = UUID_URL_BEACON_SERVICE[sizeof(UUID_URL_BEACON_SERVICE) - i - 1];
        }
        ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_128BIT_SERVICE_IDS, reversedServiceUUID, sizeof(reversedServiceUUID));
        ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::GENERIC_TAG);
        ble.gap().accumulateScanResponse(GapAdvertisingData::COMPLETE_LOCAL_NAME, reinterpret_cast<const uint8_t *>(&DEVICE_NAME), sizeof(DEVICE_NAME));
        ble.gap().accumulateScanResponse(
            GapAdvertisingData::TX_POWER_LEVEL,
            reinterpret_cast<uint8_t *>(&advPowerLevels[EddystoneService::TX_POWER_MODE_LOW]),
            sizeof(uint8_t));

        ble.gap().setTxPower(radioPowerLevels[txPowerMode]);
        ble.gap().setDeviceName(reinterpret_cast<const uint8_t *>(&DEVICE_NAME));
        ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
        ble.gap().setAdvertisingInterval(advConfigInterval);
        ble.gap().startAdvertising();
    }

    void lockAuthorizationCallback(GattWriteAuthCallbackParams *authParams)
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

    void unlockAuthorizationCallback(GattWriteAuthCallbackParams *authParams)
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

    void urlDataWriteAuthorizationCallback(GattWriteAuthCallbackParams *authParams)
    {
        if (lockState) {
            authParams->authorizationReply = AUTH_CALLBACK_REPLY_ATTERR_INSUF_AUTHORIZATION;
        } else if (authParams->offset != 0) {
            authParams->authorizationReply = AUTH_CALLBACK_REPLY_ATTERR_INVALID_OFFSET;
        } else {
            authParams->authorizationReply = AUTH_CALLBACK_REPLY_SUCCESS;
        }
    }

    void powerModeAuthorizationCallback(GattWriteAuthCallbackParams *authParams)
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
    void basicAuthorizationCallback(GattWriteAuthCallbackParams *authParams)
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
    void onDataWrittenCallback(const GattWriteCallbackParams *writeParams)
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
            if (tmpBeaconPeriod != beaconPeriod) {
                beaconPeriod = tmpBeaconPeriod;
                ble.gattServer().write(beaconPeriodChar->getValueHandle(), reinterpret_cast<uint8_t *>(&beaconPeriod), sizeof(uint16_t));
            }
        } else if (handle == resetChar->getValueHandle() && (*((uint8_t *)writeParams->data) != 0)) {
            /* Reset characteristics to default values */
            flags        = 0;
            txPowerMode  = TX_POWER_MODE_LOW;
            beaconPeriod = DEFAULT_BEACON_PERIOD_MSEC;

            urlFrame.setURLData(DEFAULT_URL);
            memset(lock, 0, sizeof(Lock_t));

            ble.gattServer().write(urlDataChar->getValueHandle(), urlFrame.getEncodedURLData(), urlFrame.getEncodedURLDataLength());
            ble.gattServer().write(flagsChar->getValueHandle(), &flags, sizeof(uint8_t));
            ble.gattServer().write(txPowerModeChar->getValueHandle(), &txPowerMode, sizeof(uint8_t));
            ble.gattServer().write(beaconPeriodChar->getValueHandle(), reinterpret_cast<uint8_t *>(&beaconPeriod), sizeof(uint16_t));
            ble.gattServer().write(lockChar->getValueHandle(), lock, sizeof(PowerLevels_t));
        }
    }

    uint16_t correctAdvertisementPeriod(uint16_t beaconPeriodIn) const
    {
        /* Re-map beaconPeriod to within permissible bounds if necessary. */
        if (beaconPeriodIn != 0) {
            if (beaconPeriodIn < ble.gap().getMinAdvertisingInterval()) {
                return ble.gap().getMinAdvertisingInterval();
            } else if (beaconPeriodIn > ble.gap().getMaxAdvertisingInterval()) {
                return ble.gap().getMaxAdvertisingInterval();
            }
        }
        return beaconPeriodIn;
    }

    struct TLMFrame
    {
    public:
        TLMFrame(uint8_t  tlmVersionIn           = 0,
                 uint16_t tlmBatteryVoltageIn    = 0,
                 uint16_t tlmBeaconTemperatureIn = 0x8000,
                 uint32_t tlmPduCountIn          = 0,
                 uint32_t tlmTimeSinceBootIn     = 0) :
            tlmVersion(tlmVersionIn),
            lastTimeSinceBootRead(0),
            tlmBatteryVoltage(tlmBatteryVoltageIn),
            tlmBeaconTemperature(tlmBeaconTemperatureIn),
            tlmPduCount(tlmPduCountIn),
            tlmTimeSinceBoot(tlmTimeSinceBootIn)
        {
        }

        void setTLMData(uint8_t tlmVersionIn = 0)
        {
            /* According to the Eddystone spec BatteryVoltage is 0 and
             * BeaconTemperature is 0x8000 if not supported
             */
            tlmVersion           = tlmVersionIn;
            tlmBatteryVoltage    = 0;
            tlmBeaconTemperature = 0x8000;
            tlmPduCount          = 0;
            tlmTimeSinceBoot     = 0;
        }

        void constructTLMFrame(uint8_t *rawFrame, EddystoneService &eddyService)
        {
            (void) eddyService;
            size_t index = 0;
            rawFrame[index++] = EDDYSTONE_UUID[0];                    // 16-bit Eddystone UUID
            rawFrame[index++] = EDDYSTONE_UUID[1];
            rawFrame[index++] = FRAME_TYPE_TLM;                       // Eddystone frame type = Telemetry
            rawFrame[index++] = tlmVersion;                           // TLM Version Number
            rawFrame[index++] = (uint8_t)(tlmBatteryVoltage >> 8);    // Battery Voltage[0]
            rawFrame[index++] = (uint8_t)(tlmBatteryVoltage >> 0);    // Battery Voltage[1]
            rawFrame[index++] = (uint8_t)(tlmBeaconTemperature >> 8); // Beacon Temp[0]
            rawFrame[index++] = (uint8_t)(tlmBeaconTemperature >> 0); // Beacon Temp[1]
            rawFrame[index++] = (uint8_t)(tlmPduCount >> 24);         // PDU Count [0]
            rawFrame[index++] = (uint8_t)(tlmPduCount >> 16);         // PDU Count [1]
            rawFrame[index++] = (uint8_t)(tlmPduCount >> 8);          // PDU Count [2]
            rawFrame[index++] = (uint8_t)(tlmPduCount >> 0);          // PDU Count [3]
            rawFrame[index++] = (uint8_t)(tlmTimeSinceBoot >> 24);    // Time Since Boot [0]
            rawFrame[index++] = (uint8_t)(tlmTimeSinceBoot >> 16);    // Time Since Boot [1]
            rawFrame[index++] = (uint8_t)(tlmTimeSinceBoot >> 8);     // Time Since Boot [2]
            rawFrame[index++] = (uint8_t)(tlmTimeSinceBoot >> 0);     // Time Since Boot [3]
        }

        size_t getRawFrameSize(void) const
        {
            return FRAME_SIZE_TLM + EDDYSTONE_UUID_SIZE;
        }

        void updateTimeSinceBoot(uint32_t nowInMillis)
        {
            tlmTimeSinceBoot      += (nowInMillis - lastTimeSinceBootRead) / 100;
            lastTimeSinceBootRead  = nowInMillis;
        }

        void updateBatteryVoltage(uint16_t tlmBatteryVoltageIn)
        {
            tlmBatteryVoltage = tlmBatteryVoltageIn;
        }

        void updateBeaconTemperature(uint16_t tlmBeaconTemperatureIn)
        {
            tlmBeaconTemperature = tlmBeaconTemperatureIn;
        }

        void updatePduCount(void)
        {
            tlmPduCount++;
        }

        uint16_t getBatteryVoltage(void) const
        {
            return tlmBatteryVoltage;
        }

        uint16_t getBeaconTemperature(void) const
        {
            return tlmBeaconTemperature;
        }

        uint8_t getTLMVersion(void) const
        {
            return tlmVersion;
        }

    private:
        static const uint8_t FRAME_TYPE_TLM = 0x20;
        static const uint8_t FRAME_SIZE_TLM = 14;

        uint8_t              tlmVersion;
        uint32_t             lastTimeSinceBootRead;
        uint16_t             tlmBatteryVoltage;
        uint16_t             tlmBeaconTemperature;
        uint32_t             tlmPduCount;
        uint32_t             tlmTimeSinceBoot;
    };

    class UIDFrame
    {
    public:
        UIDFrame(void)
        {
            memset(uidNamespaceID, 0, sizeof(UIDNamespaceID_t));
            memset(uidInstanceID,  0,  sizeof(UIDInstanceID_t));
        }

        UIDFrame(const UIDNamespaceID_t uidNamespaceIDIn,
                        const UIDInstanceID_t  uidInstanceIDIn)
        {
            memcpy(uidNamespaceID, uidNamespaceIDIn, sizeof(UIDNamespaceID_t));
            memcpy(uidInstanceID,  uidInstanceIDIn,  sizeof(UIDInstanceID_t));
        }

        /* TODO: We could save about 8 bytes if we have uidNamespaceID and uidInstanceID as
         * const pointers, but this seems a bit of an overkill and might cause problems when
         * initialising the program using EddystoneParams_t.
         */
        void setUIDData(const UIDNamespaceID_t *uidNamespaceIDIn, const UIDInstanceID_t *uidInstanceIDIn)
        {
            memcpy(uidNamespaceID, uidNamespaceIDIn, sizeof(UIDNamespaceID_t));
            memcpy(uidInstanceID,  uidInstanceIDIn,  sizeof(UIDInstanceID_t));
        }

        void constructUIDFrame(uint8_t *rawFrame, EddystoneService &eddyService)
        {
            size_t index = 0;

            rawFrame[index++] = EDDYSTONE_UUID[0];                                   // 16-bit Eddystone UUID
            rawFrame[index++] = EDDYSTONE_UUID[1];
            rawFrame[index++] = FRAME_TYPE_UID;                                      // 1B  Type
            rawFrame[index++] = eddyService.advPowerLevels[eddyService.txPowerMode]; // 1B  Power @ 0meter

            memcpy(rawFrame + index, uidNamespaceID, sizeof(UIDNamespaceID_t));      // 10B Namespace ID
            index += sizeof(UIDNamespaceID_t);
            memcpy(rawFrame + index, uidInstanceID, sizeof(UIDInstanceID_t));        // 6B Instance ID
            index += sizeof(UIDInstanceID_t);

            memset(rawFrame + index, 0, 2 * sizeof(uint8_t));                        // 2B RFU, which are unused
        }

        size_t getRawFrameSize(void) const
        {
            return FRAME_SIZE_UID + EDDYSTONE_UUID_SIZE;
        }

        uint8_t* getUIDNamespaceID(void)
        {
            return uidNamespaceID;
        }

        uint8_t* getUIDInstanceID(void)
        {
            return uidInstanceID;
        }

    private:
        static const uint8_t FRAME_TYPE_UID = 0x00;
        static const uint8_t FRAME_SIZE_UID = 20;

        UIDNamespaceID_t     uidNamespaceID;
        UIDInstanceID_t      uidInstanceID;
    };

    struct URLFrame
    {
    public:
        URLFrame(void)
        {
            urlDataLength = 0;
            memset(urlData, 0, sizeof(UrlData_t));
        }

        URLFrame(const char *urlDataIn)
        {
            encodeURL(urlDataIn);
        }

        URLFrame(UrlData_t urlDataIn, uint8_t urlDataLength)
        {
            if (urlDataLength > URL_DATA_MAX) {
                memcpy(urlData, urlDataIn, URL_DATA_MAX);
            } else {
                memcpy(urlData, urlDataIn, urlDataLength);
            }
        }

        void constructURLFrame(uint8_t* rawFrame, EddystoneService eddyService)
        {
            size_t index = 0;
            rawFrame[index++] = EDDYSTONE_UUID[0];                                   // 16-bit Eddystone UUID
            rawFrame[index++] = EDDYSTONE_UUID[1];
            rawFrame[index++] = FRAME_TYPE_URL;                                      // 1B  Type
            rawFrame[index++] = eddyService.advPowerLevels[eddyService.txPowerMode]; // 1B  Power @ 0meter
            memcpy(rawFrame + index, urlData, urlDataLength);                        // Encoded URL
        }

        size_t getRawFrameSize(void) const
        {
            return urlDataLength + FRAME_MIN_SIZE_URL + EDDYSTONE_UUID_SIZE;
        }

        uint8_t* getEncodedURLData(void)
        {
            return urlData;
        }

        uint8_t getEncodedURLDataLength(void) const
        {
            return urlDataLength;
        }

        void setURLData(const char *urlDataIn)
        {
            encodeURL(urlDataIn);
        }

        void setEncodedURLData(const uint8_t* urlEncodedDataIn, const uint8_t urlEncodedDataLengthIn)
        {
            urlDataLength = urlEncodedDataLengthIn;
            memcpy(urlData, urlEncodedDataIn, urlEncodedDataLengthIn);
        }

    private:
        static const uint8_t FRAME_TYPE_URL     = 0x10;
        /* Even if the URL is 0 bytes we still need to include the type and txPower i.e. 2 bytes */
        static const uint8_t FRAME_MIN_SIZE_URL = 2;

        uint8_t              urlDataLength;
        UrlData_t            urlData;

        void encodeURL(const char *urlDataIn)
        {
            const char  *prefixes[] = {
                "http://www.",
                "https://www.",
                "http://",
                "https://",
            };
            const size_t NUM_PREFIXES = sizeof(prefixes) / sizeof(char *);
            const char  *suffixes[]   = {
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

            urlDataLength = 0;
            memset(urlData, 0, sizeof(UrlData_t));

            if ((urlDataIn == NULL) || (strlen(urlDataIn) == 0)) {
                return;
            }

            /*
             * handle prefix
             */
            for (size_t i = 0; i < NUM_PREFIXES; i++) {
                size_t prefixLen = strlen(prefixes[i]);
                if (strncmp(urlDataIn, prefixes[i], prefixLen) == 0) {
                    urlData[urlDataLength++]  = i;
                    urlDataIn                      += prefixLen;
                    break;
                }
            }

            /*
             * handle suffixes
             */
            while (*urlDataIn && (urlDataLength < URL_DATA_MAX)) {
                /* check for suffix match */
                size_t i;
                for (i = 0; i < NUM_SUFFIXES; i++) {
                    size_t suffixLen = strlen(suffixes[i]);
                    if (strncmp(urlDataIn, suffixes[i], suffixLen) == 0) {
                        urlData[urlDataLength++]  = i;
                        urlDataIn                      += suffixLen;
                        break; /* from the for loop for checking against suffixes */
                    }
                }
                /* This is the default case where we've got an ordinary character which doesn't match a suffix. */
                if (i == NUM_SUFFIXES) {
                    urlData[urlDataLength++] = *urlDataIn;
                    ++urlDataIn;
                }
            }
        }
    };

    BLE                                                             &ble;
    uint32_t                                                        advConfigInterval;
    uint8_t                                                         operationMode;

    URLFrame                                                        urlFrame;
    UIDFrame                                                        uidFrame;
    TLMFrame                                                        tlmFrame;

    PowerLevels_t                                                   radioPowerLevels;
    PowerLevels_t                                                   advPowerLevels;
    bool                                                            lockState;
    bool                                                            resetFlag;
    Lock_t                                                          lock;
    Lock_t                                                          unlock;
    uint8_t                                                         flags;
    uint8_t                                                         txPowerMode;
    uint16_t                                                        beaconPeriod;

    ReadOnlyGattCharacteristic<bool>                                *lockStateChar;
    WriteOnlyArrayGattCharacteristic<uint8_t, sizeof(Lock_t)>       *lockChar;
    WriteOnlyArrayGattCharacteristic<uint8_t, sizeof(Lock_t)>       *unlockChar;
    GattCharacteristic                                              *urlDataChar;
    ReadWriteGattCharacteristic<uint8_t>                            *flagsChar;
    ReadWriteArrayGattCharacteristic<int8_t, sizeof(PowerLevels_t)> *advPowerLevelsChar;
    ReadWriteGattCharacteristic<uint8_t>                            *txPowerModeChar;
    ReadWriteGattCharacteristic<uint16_t>                           *beaconPeriodChar;
    WriteOnlyGattCharacteristic<bool>                               *resetChar;

    uint8_t                                                         *rawUrlFrame;
    uint8_t                                                         *rawUidFrame;
    uint8_t                                                         *rawTlmFrame;

    uint16_t                                                        consecFrames[NUM_EDDYSTONE_FRAMES];
    uint16_t                                                        currentConsecFrames[NUM_EDDYSTONE_FRAMES];
    uint8_t                                                         currentAdvertisedFrame;

    TlmUpdateCallback_t                                             tlmBatteryVoltageCallback;
    TlmUpdateCallback_t                                             tlmBeaconTemperatureCallback;

    Timer                                                           timeSinceBootTimer;
#ifdef YOTTA_CFG_MBED_OS
    Timeout                                                         swapAdvertisedFrameTimeout;
#endif

    GattCharacteristic                                              *charTable[TOTAL_CHARACTERISTICS];
};

#endif  /* __EDDYSTONESERVICE_H__ */
