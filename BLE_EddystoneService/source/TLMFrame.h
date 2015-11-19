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

class TLMFrame
{
public:
    TLMFrame(uint8_t  tlmVersionIn           = 0,
             uint16_t tlmBatteryVoltageIn    = 0,
             uint16_t tlmBeaconTemperatureIn = 0x8000,
             uint32_t tlmPduCountIn          = 0,
             uint32_t tlmTimeSinceBootIn     = 0);

    void setTLMData(uint8_t tlmVersionIn = 0);

    void constructTLMFrame(uint8_t *rawFrame);

    size_t getRawFrameSize(void) const;

    void updateTimeSinceBoot(uint32_t nowInMillis);

    void updateBatteryVoltage(uint16_t tlmBatteryVoltageIn);

    void updateBeaconTemperature(uint16_t tlmBeaconTemperatureIn);

    void updatePduCount(void);

    uint16_t getBatteryVoltage(void) const;

    uint16_t getBeaconTemperature(void) const;

    uint8_t getTLMVersion(void) const;

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

#endif  /* __TLMFRAME_H__ */
