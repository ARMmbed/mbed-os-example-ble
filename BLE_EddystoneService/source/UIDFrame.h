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

#ifndef __UIDFRAME_H__
#define __UIDFRAME_H__

#include <string.h>
#include "EddystoneTypes.h"

class UIDFrame
{
public:
    UIDFrame(void);

    UIDFrame(const UIDNamespaceID_t uidNamespaceIDIn, const UIDInstanceID_t  uidInstanceIDIn);

    void setUIDData(const UIDNamespaceID_t *uidNamespaceIDIn, const UIDInstanceID_t *uidInstanceIDIn);

    void constructUIDFrame(uint8_t *rawFrame, int8_t advPowerLevel);

    size_t getRawFrameSize(void) const;

    uint8_t* getUIDNamespaceID(void);

    uint8_t* getUIDInstanceID(void);

private:
    static const uint8_t FRAME_TYPE_UID = 0x00;
    static const uint8_t FRAME_SIZE_UID = 20;

    UIDNamespaceID_t     uidNamespaceID;
    UIDInstanceID_t      uidInstanceID;
};

#endif  /* __UIDFRAME_H__ */
