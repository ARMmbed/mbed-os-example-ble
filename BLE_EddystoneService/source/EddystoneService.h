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
#include "EddystoneTypes.h"
#include "URLFrame.h"
#include "UIDFrame.h"
#include "TLMFrame.h"
#include <string.h>
#ifdef YOTTA_CFG_MBED_OS
    #include "mbed-drivers/mbed.h"
#else
    #include "mbed.h"
#endif

class EddystoneService
{
public:
    static const uint16_t TOTAL_CHARACTERISTICS = 9;

    static const uint32_t DEFAULT_CONFIG_PERIOD_MSEC = 1000;
    static const uint16_t DEFAULT_BEACON_PERIOD_MSEC = 1000;

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

    enum EddystoneError_t {
        EDDYSTONE_ERROR_NONE,
        EDDYSTONE_ERROR_INVALID_BEACON_PERIOD,
        EDDYSTONE_ERROR_INVALID_CONSEC_FRAMES,
        EDDYSTONE_ERROR_INVALID_ADVERTISING_INTERVAL
    };

    enum FrameType {
        EDDYSTONE_FRAME_URL,
        EDDYSTONE_FRAME_UID,
        EDDYSTONE_FRAME_TLM,
        NUM_EDDYSTONE_FRAMES
    };

    /* Initialise the EddystoneService using parameters from persistent storage */
    EddystoneService(BLE                 &bleIn,
                     EddystoneParams_t   &paramsIn,
                     const PowerLevels_t &advPowerLevelsIn,
                     const PowerLevels_t &radioPowerLevelsIn,
                     uint32_t            advConfigIntervalIn = DEFAULT_CONFIG_PERIOD_MSEC);

    /* When using this constructor we need to call setURLData,
     * setTMLData and setUIDData to initialise values manually
     */
    EddystoneService(BLE                 &bleIn,
                     const PowerLevels_t &advPowerLevelsIn,
                     const PowerLevels_t &radioPowerLevelsIn,
                     uint32_t            advConfigIntervalIn = DEFAULT_CONFIG_PERIOD_MSEC);

    /* Setup callback to update BatteryVoltage in TLM frame */
    void onTLMBatteryVoltageUpdate(TlmUpdateCallback_t tlmBatteryVoltageCallbackIn);

    /* Setup callback to update BeaconTemperature in TLM frame */
    void onTLMBeaconTemperatureUpdate(TlmUpdateCallback_t tlmBeaconTemperatureCallbackIn);

    void setTLMData(uint8_t tlmVersionIn = 0);

    void setURLData(const char *urlDataIn);

    void setUIDData(const UIDNamespaceID_t &uidNamespaceIDIn, const UIDInstanceID_t &uidInstanceIDIn);

    EddystoneError_t startConfigService(void);

    EddystoneError_t startBeaconService(uint16_t consecUrlFramesIn = 2, uint16_t consecUidFramesIn = 2, uint16_t consecTlmFramesIn = 2);

    /* It is not the responsibility of the Eddystone implementation to store
     * the configured parameters in persistent storage since this is
     * platform-specific. So we provide this function that returns the
     * configured values that need to be stored and the main application
     * takes care of storing them.
     */
    void getEddystoneParams(EddystoneParams_t *params);

private:
    /* Helper function used only once during constructing the object to avoid
     * duplicated code.
     */
    void eddystoneConstructorHelper(const PowerLevels_t &advPowerLevelsIn,
                                    const PowerLevels_t &radioPowerLevelsIn,
                                    uint32_t            advConfigIntervalIn);

    /* When changing modes, we shutdown and init the BLE instance, so
     * this is needed to complete the initialisation task.
     */
    void bleInitComplete(BLE::InitializationCompleteCallbackContext* initContext);

    void swapAdvertisedFrame(void);

    void updateAdvertisementPacket(const uint8_t* rawFrame, size_t rawFrameLength);

    /* Helper function that calls user-defined functions to update Battery Voltage and Temperature (if available),
     * then updates the raw frame data and finally updates the actual advertised packet. This operation must be
     * done fairly often because the TLM frame TimeSinceBoot must have a 0.1 secs resolution according to the
     * Eddystone specification.
     */
    void updateRawTLMFrame(void);

    void setupBeaconService(void);

    void setupConfigService(void);

    void freeConfigCharacteristics(void);

    void freeBeaconFrames(void);

    void radioNotificationCallback(bool radioActive);

    /*
     * Internal helper function used to update the GATT database following any
     * change to the internal state of the service object.
     */
    void updateCharacteristicValues(void);

    void setupEddystoneConfigAdvertisements(void);

    void lockAuthorizationCallback(GattWriteAuthCallbackParams *authParams);

    void unlockAuthorizationCallback(GattWriteAuthCallbackParams *authParams);

    void urlDataWriteAuthorizationCallback(GattWriteAuthCallbackParams *authParams);

    void powerModeAuthorizationCallback(GattWriteAuthCallbackParams *authParams);

    template <typename T>
    void basicAuthorizationCallback(GattWriteAuthCallbackParams *authParams);

    /*
     * This callback is invoked when a GATT client attempts to modify any of the
     * characteristics of this service. Attempts to do so are also applied to
     * the internal state of this service object.
     */
    void onDataWrittenCallback(const GattWriteCallbackParams *writeParams);

    uint16_t correctAdvertisementPeriod(uint16_t beaconPeriodIn) const;

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
#ifndef YOTTA_CFG_MBED_OS
    Timeout                                                         swapAdvertisedFrameTimeout;
#endif

    GattCharacteristic                                              *charTable[TOTAL_CHARACTERISTICS];
};

#endif  /* __EDDYSTONESERVICE_H__ */
