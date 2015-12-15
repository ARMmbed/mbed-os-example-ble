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

#include "UIDFrame.h"

UIDFrame::UIDFrame(void)
{
    memset(uidNamespaceID, 0, sizeof(UIDNamespaceID_t));
    memset(uidInstanceID,  0,  sizeof(UIDInstanceID_t));
}

UIDFrame::UIDFrame(const UIDNamespaceID_t uidNamespaceIDIn, const UIDInstanceID_t  uidInstanceIDIn)
{
    memcpy(uidNamespaceID, uidNamespaceIDIn, sizeof(UIDNamespaceID_t));
    memcpy(uidInstanceID,  uidInstanceIDIn,  sizeof(UIDInstanceID_t));
}

void UIDFrame::setUIDData(const UIDNamespaceID_t &uidNamespaceIDIn, const UIDInstanceID_t &uidInstanceIDIn)
{
    memcpy(uidNamespaceID, uidNamespaceIDIn, sizeof(UIDNamespaceID_t));
    memcpy(uidInstanceID,  uidInstanceIDIn,  sizeof(UIDInstanceID_t));
}

void UIDFrame::constructUIDFrame(uint8_t *rawFrame, int8_t advPowerLevel)
{
    size_t index = 0;

    rawFrame[index++] = EDDYSTONE_UUID[0];                                   // 16-bit Eddystone UUID
    rawFrame[index++] = EDDYSTONE_UUID[1];
    rawFrame[index++] = FRAME_TYPE_UID;                                      // 1B  Type
    rawFrame[index++] = advPowerLevel;                                       // 1B  Power @ 0meter

    memcpy(rawFrame + index, uidNamespaceID, sizeof(UIDNamespaceID_t));      // 10B Namespace ID
    index += sizeof(UIDNamespaceID_t);
    memcpy(rawFrame + index, uidInstanceID, sizeof(UIDInstanceID_t));        // 6B Instance ID
    index += sizeof(UIDInstanceID_t);

    memset(rawFrame + index, 0, 2 * sizeof(uint8_t));                        // 2B RFU, which are unused
}

size_t UIDFrame::getRawFrameSize(void) const
{
    return FRAME_SIZE_UID + EDDYSTONE_UUID_SIZE;
}

uint8_t* UIDFrame::getUIDNamespaceID(void)
{
    return uidNamespaceID;
}

uint8_t* UIDFrame::getUIDInstanceID(void)
{
    return uidInstanceID;
}
