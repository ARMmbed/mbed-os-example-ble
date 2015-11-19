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

#include "TLMFrame.h"

TLMFrame::TLMFrame(uint8_t  tlmVersionIn,
                   uint16_t tlmBatteryVoltageIn,
                   uint16_t tlmBeaconTemperatureIn,
                   uint32_t tlmPduCountIn,
                   uint32_t tlmTimeSinceBootIn) :
    tlmVersion(tlmVersionIn),
    lastTimeSinceBootRead(0),
    tlmBatteryVoltage(tlmBatteryVoltageIn),
    tlmBeaconTemperature(tlmBeaconTemperatureIn),
    tlmPduCount(tlmPduCountIn),
    tlmTimeSinceBoot(tlmTimeSinceBootIn)
{
}

void TLMFrame::setTLMData(uint8_t tlmVersionIn)
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

void TLMFrame::constructTLMFrame(uint8_t *rawFrame)
{
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

size_t TLMFrame::getRawFrameSize(void) const
{
    return FRAME_SIZE_TLM + EDDYSTONE_UUID_SIZE;
}

void TLMFrame::updateTimeSinceBoot(uint32_t nowInMillis)
{
    tlmTimeSinceBoot      += (nowInMillis - lastTimeSinceBootRead) / 100;
    lastTimeSinceBootRead  = nowInMillis;
}

void TLMFrame::updateBatteryVoltage(uint16_t tlmBatteryVoltageIn)
{
    tlmBatteryVoltage = tlmBatteryVoltageIn;
}

void TLMFrame::updateBeaconTemperature(uint16_t tlmBeaconTemperatureIn)
{
    tlmBeaconTemperature = tlmBeaconTemperatureIn;
}

void TLMFrame::updatePduCount(void)
{
    tlmPduCount++;
}

uint16_t TLMFrame::getBatteryVoltage(void) const
{
    return tlmBatteryVoltage;
}

uint16_t TLMFrame::getBeaconTemperature(void) const
{
    return tlmBeaconTemperature;
}

uint8_t TLMFrame::getTLMVersion(void) const
{
    return tlmVersion;
}
