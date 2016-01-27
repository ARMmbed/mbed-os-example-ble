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

#ifndef __TLMFRAME_H__
#define __TLMFRAME_H__

#include "EddystoneTypes.h"

/**
 * Class that encapsulates data that belongs to the Eddystone-TLM frame. For
 * more information refer to https://github.com/google/eddystone/tree/master/eddystone-tlm.
 */
class TLMFrame
{
public:
    /**
     * Construct a new instance of this class.
     *
     * @param[in] tlmVersionIn
     *              Eddystone-TLM version number to use.
     * @param[in] tlmBatteryVoltageIn
     *              Initial value for the Eddystone-TLM Battery Voltage.
     * @param[in] tlmBeaconTemperatureIn
     *              Initial value for the Eddystone-TLM Beacon Temperature.
     * @param[in] tlmPduCountIn
     *              Initial value for the Eddystone-TLM Advertising PDU Count.
     * @param[in] tlmTimeSinceBootIn
     *              Intitial value for the Eddystone-TLM time since boot timer.
     8              This timer has a 0.1 second resolution.
     */
    TLMFrame(uint8_t  tlmVersionIn           = 0,
             uint16_t tlmBatteryVoltageIn    = 0,
             uint16_t tlmBeaconTemperatureIn = 0x8000,
             uint32_t tlmPduCountIn          = 0,
             uint32_t tlmTimeSinceBootIn     = 0);

    /**
     * Set the Eddystone-TLM version number.
     */
    void setTLMData(uint8_t tlmVersionIn = 0);

    /**
     * Construct the raw bytes of the Eddystone-TLM frame that will be directly
     * used in the advertising packets.
     *
     * @param[in] rawFrame
     *              Pointer to the location where the raw frame will be stored.
     */
    void constructTLMFrame(uint8_t *rawFrame);

    /**
     * Get the size of the Eddystone-TLM frame constructed with the
     * current state of the TLMFrame object.
     *
     * @return The size in bytes of the Eddystone-TLM frame.
     */
    size_t getRawFrameSize(void) const;

    /**
     * Update the time since boot.
     *
     * @param[in] nowInMillis
     *              The time since boot in milliseconds.
     */
    void updateTimeSinceBoot(uint32_t nowInMillis);

    /**
     * Update the Battery Voltage.
     *
     * @param[in] tlmBatteryVoltageIn
     *              The new Battery Voltage value.
     */
    void updateBatteryVoltage(uint16_t tlmBatteryVoltageIn);

    /**
     * Update the Beacon Temperature.
     *
     * @param[in] tlmBeaconTemperatureIn
     *              The new Beacon Temperature value.
     */
    void updateBeaconTemperature(uint16_t tlmBeaconTemperatureIn);

    /**
     * Increment the current PDU counter by 1.
     */
    void updatePduCount(void);

    /**
     * Get the current Battery Voltage.
     *
     * @return The Battery Voltage.
     */
    uint16_t getBatteryVoltage(void) const;

    /**
     * Get the current Beacon Temperature.
     *
     * @return The Beacon Temperature.
     */
    uint16_t getBeaconTemperature(void) const;

    /**
     * Get the current TLM Version number.
     *
     * @return The TLM Version number.
     */
    uint8_t getTLMVersion(void) const;

private:
    /**
     * The byte ID of an Eddystone-TLM frame.
     */
    static const uint8_t FRAME_TYPE_TLM = 0x20;
    /**
     * The size of an Eddystone-TLM frame.
     */
    static const uint8_t FRAME_SIZE_TLM = 14;

    /**
     * Eddystone-TLM version value.
     */
    uint8_t              tlmVersion;
    /**
     * Time since boot in milliseconds.
     */
    uint32_t             lastTimeSinceBootRead;
    /**
     * Eddystone-TLM Battery Voltage value.
     */
    uint16_t             tlmBatteryVoltage;
    /**
     * Eddystone-TLM Beacon temperature value.
     */
    uint16_t             tlmBeaconTemperature;
    /**
     * Eddystone-TLM Advertising PDU Count.
     */
    uint32_t             tlmPduCount;
    /**
     * Eddystone-TLM time since boot with 0.1 second resolution.
     */
    uint32_t             tlmTimeSinceBoot;
};

#endif  /* __TLMFRAME_H__ */
