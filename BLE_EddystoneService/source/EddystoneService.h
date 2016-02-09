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
    #include "mbed-drivers/CircularBuffer.h"
#else
    #include "mbed.h"
    #include "CircularBuffer.h"
#endif

#ifndef YOTTA_CFG_DEFAULT_URL_FRAME_INTERVAL
    #define YOTTA_CFG_DEFAULT_URL_FRAME_INTERVAL 700
#endif

#ifndef YOTTA_CFG_DEFAULT_UID_FRAME_INTERVAL
    #define YOTTA_CFG_DEFAULT_UID_FRAME_INTERVAL 300
#endif

#ifndef YOTTA_CFG_DEFAULT_TLM_FRAME_INTERVAL
    #define YOTTA_CFG_DEFAULT_TLM_FRAME_INTERVAL 2000
#endif

#ifndef YOTTA_CFG_DEFAULT_EDDYSTONE_URL_CONFIG_ADV_INTERVAL
    #define YOTTA_CFG_DEFAULT_EDDYSTONE_URL_CONFIG_ADV_INTERVAL 1000
#endif

/**
 * This class implements the Eddystone-URL Config Service and the Eddystone
 * Protocol Specification as defined in the publicly available specification at
 * https://github.com/google/eddystone/blob/master/protocol-specification.md.
 */
class EddystoneService
{
public:
    /**
     * Total number of GATT Characteristics in the Eddystonei-URL Configuration
     * Service.
     */
    static const uint16_t TOTAL_CHARACTERISTICS = 9;

    /**
     * Default interval for advertising packets for the Eddystone-URL
     * Configuration Service.
     */
    static const uint32_t DEFAULT_CONFIG_PERIOD_MSEC    = YOTTA_CFG_DEFAULT_EDDYSTONE_URL_CONFIG_ADV_INTERVAL;
    /**
     * Recommended interval for advertising packets containing Eddystone URL
     * frames.
     */
    static const uint16_t DEFAULT_URL_FRAME_PERIOD_MSEC = YOTTA_CFG_DEFAULT_URL_FRAME_INTERVAL;
    /**
     * Recommended interval for advertising packets containing Eddystone UID
     * frames.
     */
    static const uint16_t DEFAULT_UID_FRAME_PERIOD_MSEC = YOTTA_CFG_DEFAULT_UID_FRAME_INTERVAL;
    /**
     * Recommended interval for advertising packets containing Eddystone TLM
     * frames.
     */
    static const uint16_t DEFAULT_TLM_FRAME_PERIOD_MSEC = YOTTA_CFG_DEFAULT_TLM_FRAME_INTERVAL;

    /**
     * Enumeration that defines the various operation modes of the
     * EddystoneService.
     *
     * @note The main app can change the mode of EddystoneService at any point
     *       of time by calling startConfigService() or startBeaconService().
     *       Resources from the previous mode will be freed.
     *
     * @note It is currently NOT possible to force EddystoneService back into
     *       EDDYSTONE_MODE_NONE.
     */
    enum OperationModes {
        /**
         * NONE: EddystoneService has been initialized but no memory has been
         * dynamically allocated. Additionally, no services are running
         * nothing is being advertised.
         */
        EDDYSTONE_MODE_NONE,
        /**
         * CONFIG: EddystoneService has been initialized, the configuration
         *         service started and memory has been allocated for BLE
         *         characteristics. Memory consumption peaks during CONFIG
         *         mode.
         */
        EDDYSTONE_MODE_CONFIG,
        /**
         * BEACON: Eddystone service is running as a beacon advertising URL,
         *         UID and/or TLM frames depending on how it is configured.
         */
        EDDYSTONE_MODE_BEACON
    };

    /**
     * Structure that encapsulates the Eddystone configuration parameters. This
     * structure is particularly useful when storing the parameters to
     * persistent storage.
     */
    struct EddystoneParams_t {
        /**
         * The value of the Eddystone-URL Configuration Service Lock State
         * characteristic.
         */
        bool             lockState;
        /**
         * The value of the Eddystone-URL Configuration Service Lock
         * characteristic that can be used to lock the beacon and set the
         * single-use lock-code.
         */
        Lock_t           lock;
        /**
         * The value of the Eddystone-URL Configuration Service Unlock
         * characteristic that can be used to unlock the beacon and clear the
         * single-use lock-code.
         */
        Lock_t           unlock;
        /**
         * The value of the Eddystone-URL Configuration Service Flags
         * characteristic. This value is currently fixed to 0x10.
         */
        uint8_t          flags;
        /**
         * The value of the Eddystone-URL Configuration Service Advertised TX
         * Power Levels characteristic that is an array of bytes whose values
         * are put into the advertising packets when in EDDYSTONE_BEACON_MODE.
         *
         * @note These are not the same values set internally into the radio tx
         *       power.
         */
        PowerLevels_t    advPowerLevels;
        /**
         * The value of the Eddystone-URL Configuration Service TX Power Mode
         * characteristic. This value is an index into the
         * EddystoneParams_t::advPowerLevels array.
         */
        uint8_t          txPowerMode;
        /**
         * The value of the Eddystone-URL Configuration Service Beacon Period
         * characteristic that is the interval (in milliseconds) of the
         * Eddystone-URL frames.
         *
         * @note A value of zero disables Eddystone-URL frame trasmissions.
         */
        uint16_t         urlFramePeriod;
        /**
         * The configured interval (in milliseconds) of the Eddystone-UID
         * frames.
         *
         * @note A value of zero disables Eddystone-UID frame transmissions.
         *
         * @note Currently it is only possible to modify this value by using
         *       the setUIDFrameAdvertisingInterval() API.
         */
        uint16_t         uidFramePeriod;
        /**
         * The configured interval (in milliseconds) of the Eddystone-TLM
         * frames.
         *
         * @note A value of zero disables Eddystone-TLM frame transmissions.
         *
         * @note Currently it is only possible to modify this value by using
         *       the setTLMFrameAdvertisingInterval() API.
         */
        uint16_t         tlmFramePeriod;
        /**
         * The configured version of the Eddystone-TLM frames.
         */
        uint8_t          tlmVersion;
        /**
         * The length of the encoded URL in EddystoneParams_t::urlData used
         * within Eddystone-URL frames.
         */
        uint8_t          urlDataLength;
        /**
         * The value of the Eddystone-URL Configuration Service URI Data
         * characteristic that contains an encoded URL as described in the
         * Eddystone Specification at
         * https://github.com/google/eddystone/blob/master/eddystone-url/README.md#eddystone-url-http-url-encoding.
         */
        UrlData_t        urlData;
        /**
         * The configured 10-byte namespace ID in Eddystone-UID frames that may
         * be used to group a particular set of beacons.
         */
        UIDNamespaceID_t uidNamespaceID;
        /**
         * The configured 6-byte instance ID that may be used to uniquely
         * identify individual devices in a group.
         */
        UIDInstanceID_t  uidInstanceID;
    };

    /**
     * Enumeration that defines the various error codes for EddystoneService.
     */
    enum EddystoneError_t {
        /**
         * No error occurred.
         */
        EDDYSTONE_ERROR_NONE,
        /**
         * The supplied advertising interval is invalid. The interval may be
         * too short/long for the type of advertising packets being broadcast.
         *
         * @note For the acceptable range of advertising interval refer to the
         *       following functions in mbed BLE API:
         *       - Gap::getMinNonConnectableAdvertisingInterval()
         *       - Gap::getMinAdvertisingInterval()
         *       - Gap::getMaxAdvertisingInterval()
         */
        EDDYSTONE_ERROR_INVALID_ADVERTISING_INTERVAL,
        /**
         * The result of executing a call when the the EddystoneService is in
         * the incorrect operation mode.
         */
        EDDYSTONE_ERROR_INVALID_STATE
    };

    /**
     * Enumeration that defines the available frame types within Eddystone
     * advertising packets.
     */
    enum FrameType {
        /**
         * The Eddystone-URL frame. Refer to
         * https://github.com/google/eddystone/tree/master/eddystone-url.
         */
        EDDYSTONE_FRAME_URL,
        /**
         * The Eddystone-URL frame. Refer to
         * https://github.com/google/eddystone/tree/master/eddystone-uid.
         */
        EDDYSTONE_FRAME_UID,
        /**
         * The Eddystone-URL frame. Refer to
         * https://github.com/google/eddystone/tree/master/eddystone-tlm.
         */
        EDDYSTONE_FRAME_TLM,
        /**
         * The total number Eddystone frame types.
         */
        NUM_EDDYSTONE_FRAMES
    };

    /**
     * The size of the advertising frame queue.
     *
     * @note [WARNING] If the advertising rate for any of the frames is higher
     *       than 100ms then frames will be dropped, this value must be
     *       increased.
     */
    static const uint16_t ADV_FRAME_QUEUE_SIZE = NUM_EDDYSTONE_FRAMES;


    /**
     * Constructor that Initializes the EddystoneService using parameters from
     * the supplied EddystoneParams_t. This constructor is particularly useful
     * for configuring the EddystoneService with parameters fetched from
     * persistent storage.
     *
     * @param[in] bleIn
     *              The BLE instance.
     * @param[in] paramIn
     *              The input Eddystone configuration parameters.
     * @param[in] radioPowerLevelsIn
     *              The value set internally into the radion tx power.
     * @param[in] advConfigIntervalIn
     *              The advertising interval for advertising packets of the
     *              Eddystone-URL Configuration Service.
     */
    EddystoneService(BLE                 &bleIn,
                     EddystoneParams_t   &paramsIn,
                     const PowerLevels_t &radioPowerLevelsIn,
                     uint32_t            advConfigIntervalIn = DEFAULT_CONFIG_PERIOD_MSEC);

    /**
     * Constructor to initialize the EddystoneService to default values.
     *
     * @param[in] bleIn
     *              The BLE instance.
     * @param[in] advPowerLevelsIn
     *              The value of the Eddystone-URL Configuration Service TX
     *              Power Mode characteristic.
     * @param[in] radioPowerLevelsIn
     *              The value set internally into the radion tx power.
     * @param[in] advConfigIntervalIn
     *              The advertising interval for advertising packets of the
     *              Eddystone-URL Configuration Service.
     *
     * @note When using this constructor the setURLData(), setTMLData() and
     *       setUIDData() functions must be called to initialize
     *       EddystoneService manually.
     */
    EddystoneService(BLE                 &bleIn,
                     const PowerLevels_t &advPowerLevelsIn,
                     const PowerLevels_t &radioPowerLevelsIn,
                     uint32_t            advConfigIntervalIn = DEFAULT_CONFIG_PERIOD_MSEC);

    /**
     * Setup callback to update BatteryVoltage in Eddystone-TLM frames
     *
     * @param[in] tlmBatteryVoltageCallbackIn
     *              The callback being registered.
     */
    void onTLMBatteryVoltageUpdate(TlmUpdateCallback_t tlmBatteryVoltageCallbackIn);

    /**
     * Setup callback to update BeaconTemperature in Eddystone-TLM frames
     *
     * @param[in] tlmBeaconTemperatureCallbackIn
     *              The callback being registered.
     */
    void onTLMBeaconTemperatureUpdate(TlmUpdateCallback_t tlmBeaconTemperatureCallbackIn);

    /**
     * Set the Eddystone-TLM frame version. The other components of
     * Eddystone-TLM frames are updated just before the frame is broadcast
     * since information such as beacon temperature and time since boot changes
     * relatively quickly.
     *
     * @param[in] tlmVersionIn
     *              The Eddyston-TLM version to set.
     */
    void setTLMData(uint8_t tlmVersionIn = 0);

    /**
     * Set the Eddystone-URL frame URL data.
     *
     * @param[in] urlDataIn
     *              A pointer to the plain null terminated string representing
     *              a URL to be encoded.
     */
    void setURLData(const char *urlDataIn);

    /**
     * Set the Eddystone-UID namespace and instance IDs.
     *
     * @param[in] uidNamespaceIDIn
     *              The new Eddystone-UID namespace ID.
     * @param[in] uidInstanceIDIn
     *              The new Eddystone-UID instance ID.
     */
    void setUIDData(const UIDNamespaceID_t &uidNamespaceIDIn, const UIDInstanceID_t &uidInstanceIDIn);

    /**
     * Set the interval of the Eddystone-URL frames.
     *
     * @param[in] urlFrameIntervalIn
     *              The new frame interval in milliseconds. The default is
     *              DEFAULT_URL_FRAME_PERIOD_MSEC.
     *
     * @note A value of zero disables Eddystone-URL frame transmissions.
     */
    void setURLFrameAdvertisingInterval(uint16_t urlFrameIntervalIn = DEFAULT_URL_FRAME_PERIOD_MSEC);

    /**
     * Set the interval of the Eddystone-UID frames.
     *
     * @param[in] uidFrameIntervalIn
     *              The new frame interval in milliseconds. The default is
     *              DEFAULT_UID_FRAME_PERIOD_MSEC.
     *
     * @note A value of zero disables Eddystone-UID frame transmissions.
     */
    void setUIDFrameAdvertisingInterval(uint16_t uidFrameIntervalIn = DEFAULT_UID_FRAME_PERIOD_MSEC);

    /**
     * Set the interval for the Eddystone-TLM frames.
     *
     * @param[in] tlmFrameIntervalIn
     *              The new frame interval in milliseconds. The default is
     *              DEFAULT_TLM_FRAME_PERIOD_MSEC.
     *
     * @note A value of zero desables Eddystone-TLM frames.
     */
    void setTLMFrameAdvertisingInterval(uint16_t tlmFrameIntervalIn = DEFAULT_TLM_FRAME_PERIOD_MSEC);

    /**
     * Change the EddystoneService OperationMode to EDDYSTONE_MODE_CONFIG.
     *
     * @retval EDDYSTONE_ERROR_NONE if the operation succeeded.
     * @retval EDDYSONE_ERROR_INVALID_ADVERTISING_INTERVAL if the configured
     *         advertising interval is zero.
     *
     * @note If EddystoneService was previously in EDDYSTONE_MODE_BEACON, then
     *       the resources allocated to that mode of operation such as memory
     *       are freed and the BLE instance shutdown before the new operation
     *       mode is configured.
     */
    EddystoneError_t startConfigService(void);

    /**
     * Change the EddystoneService OperationMode to EDDYSTONE_MODE_BEACON.
     *
     * @retval EDDYSTONE_ERROR_NONE if the operation succeeded.
     * @retval EDDYSONE_ERROR_INVALID_ADVERTISING_INTERVAL if the configured
     *         advertising interval is zero.
     *
     * @note If EddystoneService was previously in EDDYSTONE_MODE_CONFIG, then
     *       the resources allocated to that mode of operation such as memory
     *       are freed and the BLE instance shutdown before the new operation
     *       mode is configured.
     */
    EddystoneError_t startBeaconService(void);

    /**
     * Change the EddystoneService OperationMode to EDDYSTONE_MODE_NONE.
     *
     * @retval EDDYSTONE_ERROR_NONE if the operation succeeded.
     * @retval EDDYSTONE_ERROR_INVALID_STATE if the state of the
     *         EddystoneService already is EDDYSTONE_MODE_NONE.
     *
     * @note If EddystoneService was previously in EDDYSTONE_MODE_CONFIG or
     *       EDDYSTONE_MODE_BEACON, then the resources allocated to that mode
     *       of operation such as memory are freed and the BLE instance
     *       shutdown before the new operation mode is configured.
     */
    EddystoneError_t stopCurrentService(void);

    /**
     * Set the Comple Local Name for the BLE device. This not only updates
     * the value of the Device Name Characteristic, it also updates the scan
     * response payload if the EddystoneService is currently in
     * EDDYSTONE_MODE_CONFIG.
     *
     * @param[in] deviceNameIn
     *              A pointer to a null terminated string containing the new
     *              device name.
     *
     * @return BLE_ERROR_NONE if the name was successfully set. Otherwise an
     *         appropriate error.
     *
     * @note EddystoneService does not make an internal copy of the string
     *       pointed to by @p deviceNameIn. Therefore, the user is responsible
     *       for ensuring that the string persists in memory as long as it is
     *       in use by the EddystoneService.
     *
     * @note The device name is not considered an Eddystone configuration
     *       parameter; therefore, it is not contained within the
     *       EddystoneParams_t structure and must be stored to persistent
     *       storage separately.
     */
    ble_error_t setCompleteDeviceName(const char *deviceNameIn);

    /**
     * Get the Eddystone Configuration parameters. This is particularly useful
     * for storing the configuration parameters in persistent storage.
     * It is not the responsibility of the Eddystone implementation to store
     * the configured parameters in persistent storage since this is
     * platform-specific.
     *
     * @param[out] params
     *              A reference to an EddystoneParams_t structure with the
     *              configured parameters of the EddystoneService.
     */
    void getEddystoneParams(EddystoneParams_t &params);

private:
    /**
     * Helper function used only once during construction of an
     * EddystoneService object to avoid duplicated code.
     *
     * @param[in] advPowerLevelsIn
     *              The value of the Eddystone-URL Configuration Service TX
     *              Power Mode characteristic.
     * @param[in] radioPowerLevelsIn
     *              The value set internally into the radion tx power.
     * @param[in] advConfigIntervalIn
     *              The advertising interval for advertising packets of the
     *              Eddystone-URL Configuration Service.
     */
    void eddystoneConstructorHelper(const PowerLevels_t &advPowerLevelsIn,
                                    const PowerLevels_t &radioPowerLevelsIn,
                                    uint32_t            advConfigIntervalIn);

    /**
     * Helper funtion that will be registered as an initialization complete
     * callback when BLE::shutdown() is called. This is necessary when changing
     * Eddystone OperationModes. Once the BLE initialization is complete, this
     * callback will initialize all the necessary resource to operate
     * Eddystone service in the selected mode.
     *
     * @param[in] initContext
     *              The context provided by BLE API when initialization
     *              completes.
     */
    void bleInitComplete(BLE::InitializationCompleteCallbackContext* initContext);

    /**
     * When in EDDYSTONE_MODE_BEACON this function is called to update the
     * advertising payload to contain the information related to the specified
     * FrameType.
     *
     * @param[in] frameType
     *              The frame to populate the advertising payload with.
     */
    void swapAdvertisedFrame(FrameType frameType);

    /**
     * Helper function that manages the BLE radio that is used to broadcast
     * advertising packets. To advertise frames at the configured intervals
     * the actual advertising interval of the BLE instance is set to the value
     * returned by Gap::getMaxAdvertisingInterval() from the BLE API. When a
     * frame needs to be advertised, the enqueueFrame() callbacks add the frame
     * type to the advFrameQueue and post a manageRadio() callback. When the
     * callback is executed, the frame is dequeued and advertised using the
     * radio (by updating the advertising payload). manageRadio() also posts a
     * callback to itself Gap::getMinNonConnectableAdvertisingInterval()
     * milliseconds later. In this callback, manageRadio() will advertise the
     * next frame in the queue, yet if there is none it calls
     * Gap::stopAdvertising() and does not post any further callbacks.
     */
    void manageRadio(void);

    /**
     * Regular callbacks posted at the rate of urlFramePeriod, uidFramePeriod
     * and tlmFramePeriod milliseconds enqueue frames to be advertised. If the
     * frame queue is currently empty, then this function directly calls
     * manageRadio() to broadcast the required FrameType.
     *
     * @param[in] frameType
     *              The FrameType to enqueue for broadcasting.
     */
    void enqueueFrame(FrameType frameType);

    /**
     * Helper function that updates the advertising payload when in
     * EDDYSTONE_MODE_BEACON to contain a new frame.
     *
     * @param[in] rawFrame
     *              The raw bytes of the frame to advertise.
     * @param[in] rawFrameLength
     *              The length in bytes of the array pointed to by @p rawFrame.
     */
    void updateAdvertisementPacket(const uint8_t* rawFrame, size_t rawFrameLength);

    /**
     * Helper function that updates the information in the Eddystone-TLM frames
     * Internally, this function executes the registered callbacks to update
     * beacon Battery Voltage and Temperature (if available). Furthermore, this
     * function updates the raw frame data. This operation must be done fairly
     * often because the Eddystone-TLM frame Time Since Boot must have a 0.1
     * seconds resolution according to the Eddystone specification.
     */
    void updateRawTLMFrame(void);

    /**
     * Initialize the resources required when switching to
     * EDDYSTONE_MODE_BEACON.
     */
    void setupBeaconService(void);

    /**
     * Initialize the resources required when switching to
     * EDDYSTONE_MODE_CONFIG. This includes the GATT services and
     * characteristics required by the Eddystone-URL Configuration Service.
     */
    void setupConfigService(void);

    /**
     * Free the resources acquired by a call to setupConfigService().
     */
    void freeConfigCharacteristics(void);

    /**
     * Free the resources acquired by a call to setupBeaconService() and
     * cancel all pending callbacks that operate the radio and frame queue.
     *
     * @note This call will not modify the current state of the BLE device.
     *       EddystoneService::stopBeaconService should only be called after
     *       a call to BLE::shutdown().
     */
    void stopBeaconService(void);

    /**
     * Helper function used to update the GATT database following any
     * change to the internal state of the service object.
     */
    void updateCharacteristicValues(void);

    /**
     * Setup the payload of advertising packets for Eddystone-URL Configuration
     * Service.
     */
    void setupEddystoneConfigAdvertisements(void);

    /**
     * Helper function to setup the payload of scan response packets for
     * Eddystone-URL Configuration Service.
     */
    void setupEddystoneConfigScanResponse(void);

    /**
     * Callback registered to the BLE API to authorize write operations to the
     * Eddystone-URL Configuration Service Lock characteristic.
     *
     * @param[in] authParams
     *              Write authentication information.
     */
    void lockAuthorizationCallback(GattWriteAuthCallbackParams *authParams);

    /**
     * Callback registered to the BLE API to authorize write operations to the
     * Eddystone-URL Configuration Service Unlock characteristic.
     *
     * @param[in] authParams
     *              Write authentication information.
     */
    void unlockAuthorizationCallback(GattWriteAuthCallbackParams *authParams);

    /**
     * Callback registered to the BLE API to authorize write operations to the
     * Eddystone-URL Configuration Service URI Data characteristic.
     *
     * @param[in] authParams
     *              Write authentication information.
     */
    void urlDataWriteAuthorizationCallback(GattWriteAuthCallbackParams *authParams);

    void powerModeAuthorizationCallback(GattWriteAuthCallbackParams *authParams);

    /**
     * Callback registered to the BLE API to authorize write operations to the
     * following Eddystone-URL Configuration Service characteristics:
     * - Flags
     * - Beacon Period
     * - Reset
     *
     * @param[in] authParams
     *              Write authentication information.
     */
    template <typename T>
    void basicAuthorizationCallback(GattWriteAuthCallbackParams *authParams);

    /**
     * This callback is invoked when a GATT client attempts to modify any of the
     * characteristics of this service. Attempts to do so are also applied to
     * the internal state of this service object.
     *
     * @param[in] writeParams
     *              Information about the values that are being written.
     */
    void onDataWrittenCallback(const GattWriteCallbackParams *writeParams);

    /**
     * Correct the advertising interval for non-connectable packets.
     *
     * @param[in] beaconPeriodIn
     *              The input interval in milliseconds.
     *
     * @return The corrected interval in milliseconds.
     *
     * @note For the acceptable range of advertising interval refer to the
     *       following functions in mbed BLE API:
     *       - Gap::getMinNonConnectableAdvertisingInterval()
     *       - Gap::getMaxAdvertisingInterval()
     */
    uint16_t correctAdvertisementPeriod(uint16_t beaconPeriodIn) const;

    /**
     * BLE instance that EddystoneService will operate on.
     */
    BLE                                                             &ble;
    /**
     * The advertising interval for Eddystone-URL Config Service advertising
     * packets.
     */
    uint32_t                                                        advConfigInterval;
    /**
     * Current EddystoneServce operation mode.
     */
    uint8_t                                                         operationMode;

    /**
     * Encapsulation of a URL frame.
     */
    URLFrame                                                        urlFrame;
    /**
     * Encapsulation of a UID frame.
     */
    UIDFrame                                                        uidFrame;
    /**
     * Encapsulation of a TLM frame.
     */
    TLMFrame                                                        tlmFrame;

    /**
     * The value set internally into the radion tx power.
     */
    PowerLevels_t                                                   radioPowerLevels;
    /**
     * An array containing possible values for advertised tx power in Eddystone
     * frames. Also, the value of the Eddystone-URL Configuration Service
     * Advertised TX Power Levels characteristic.
     */
    PowerLevels_t                                                   advPowerLevels;
    /**
     * The value of the Eddystone-URL Configuration Service Lock State
     * characteristic.
     */
    bool                                                            lockState;
    /**
     * The value of the Eddystone-URL Configuration Service reset
     * characteristic.
     */
    bool                                                            resetFlag;
    /**
     * The value of the Eddystone-URL Configuration Service Lock
     * characteristic.
     */
    Lock_t                                                          lock;
    /**
     * The value of the Eddystone-URL Configuration Service Unlock
     * characteristic.
     */
    Lock_t                                                          unlock;
    /**
     * The value of the Eddystone-URL Configuration Service Flags
     * characteristic.
     */
    uint8_t                                                         flags;
    /**
     * The value of the Eddystone-URL Configuration Service TX Power Mode
     * characteristic.
     */
    uint8_t                                                         txPowerMode;
    /**
     * The value of the Eddystone-URL Configuration Service Beacon Period
     * characteristic. Also, the advertising interval (in milliseconds) of
     * Eddystone-URL frames.
     */
    uint16_t                                                        urlFramePeriod;
    /**
     * The advertising interval (in milliseconds) of Eddystone-UID frames.
     */
    uint16_t                                                        uidFramePeriod;
    /**
     * The advertising interval (in milliseconds) of Eddystone-TLM frames.
     */
    uint16_t                                                        tlmFramePeriod;

    /**
     * Pointer to the BLE API characteristic encapsulation for the Eddystone-URL
     * Configuration Service Lock State characteristic.
     */
    ReadOnlyGattCharacteristic<bool>                                *lockStateChar;
    /**
     * Pointer to the BLE API characteristic encapsulation for the Eddystone-URL
     * Configuration Service Lock characteristic.
     */
    WriteOnlyArrayGattCharacteristic<uint8_t, sizeof(Lock_t)>       *lockChar;
    /**
     * Pointer to the BLE API characteristic encapsulation for the Eddystone-URL
     * Configuration Service Unlock characteristic.
     */
    WriteOnlyArrayGattCharacteristic<uint8_t, sizeof(Lock_t)>       *unlockChar;
    /**
     * Pointer to the BLE API characteristic encapsulation for the Eddystone-URL
     * Configuration Service URI Data characteristic.
     */
    GattCharacteristic                                              *urlDataChar;
    /**
     * Pointer to the BLE API characteristic encapsulation for the Eddystone-URL
     * Configuration Service Flags characteristic.
     */
    ReadWriteGattCharacteristic<uint8_t>                            *flagsChar;
    /**
     * Pointer to the BLE API characteristic encapsulation for the Eddystone-URL
     * Configuration Service Advertised TX Power Levels characteristic.
     */
    ReadWriteArrayGattCharacteristic<int8_t, sizeof(PowerLevels_t)> *advPowerLevelsChar;
    /**
     * Pointer to the BLE API characteristic encapsulation for the Eddystone-URL
     * Configuration Service TX Power Mode characteristic.
     */
    ReadWriteGattCharacteristic<uint8_t>                            *txPowerModeChar;
    /**
     * Pointer to the BLE API characteristic encapsulation for the Eddystone-URL
     * Configuration Service Beacon Period characteristic.
     */
    ReadWriteGattCharacteristic<uint16_t>                           *beaconPeriodChar;
    /**
     * Pointer to the BLE API characteristic encapsulation for the Eddystone-URL
     * Configuration Service Reset characteristic.
     */
    WriteOnlyGattCharacteristic<bool>                               *resetChar;

    /**
     * Pointer to the raw bytes that will be used to populate Eddystone-URL
     * frames.
     */
    uint8_t                                                         *rawUrlFrame;
    /**
     * Pointer to the raw bytes that will be used to populate Eddystone-UID
     * frames.
     */
    uint8_t                                                         *rawUidFrame;
    /**
     * Pointer to the raw bytes that will be used to populate Eddystone-TLM
     * frames.
     */
    uint8_t                                                         *rawTlmFrame;

    /**
     * Circular buffer that represents of Eddystone frames to be advertised.
     */
    CircularBuffer<FrameType, ADV_FRAME_QUEUE_SIZE>                 advFrameQueue;

    /**
     * The registered callback to update the Eddystone-TLM frame Battery
     * Voltage.
     */
    TlmUpdateCallback_t                                             tlmBatteryVoltageCallback;
    /**
     * The registered callback to update the Eddystone-TLM frame Beacon
     * Temperature.
     */
    TlmUpdateCallback_t                                             tlmBeaconTemperatureCallback;

    /**
     * Timer that keeps track of the time since boot.
     */
    Timer                                                           timeSinceBootTimer;

    /**
     * Minar callback handle to keep track of periodic
     * enqueueFrame(EDDYSTONE_FRAME_UID) callbacks that populate the
     * advFrameQueue.
     */
    minar::callback_handle_t                                        uidFrameCallbackHandle;
    /**
     * Minar callback handle to keep track of periodic
     * enqueueFrame(EDDYSTONE_FRAME_URL) callbacks that populate the
     * advFrameQueue.
     */
    minar::callback_handle_t                                        urlFrameCallbackHandle;
    /**
     * Minar callback handle to keep track of periodic
     * enqueueFrame(EDDYSTONE_FRAME_TLM) callbacks that populate the
     * advFrameQueue.
     */
    minar::callback_handle_t                                        tlmFrameCallbackHandle;
    /**
     * Minar callback handle to keep track of manageRadio() callbacks.
     */
    minar::callback_handle_t                                        radioManagerCallbackHandle;

    /**
     * GattCharacteristic table used to populate the BLE ATT table in the
     * GATT Server.
     */
    GattCharacteristic                                              *charTable[TOTAL_CHARACTERISTICS];

    /**
     * Pointer to the device name currently being used.
     */
    const char                                                      *deviceName;
};

#endif  /* __EDDYSTONESERVICE_H__ */
