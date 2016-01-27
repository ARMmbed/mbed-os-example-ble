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

/**
 * Class that encapsulates data that belongs to the Eddystone-UID frame. For
 * more information refer to https://github.com/google/eddystone/tree/master/eddystone-uid.
 */
class UIDFrame
{
public:
    /**
     * Construct a new instance of this class.
     */
    UIDFrame(void);

    /**
     * Construct a new instance of this class.
     *
     * @param[in] uidNamespaceIDIn
     *              The Eddystone-UID namespace ID.
     * @param[in] uidInstanceIDIn
     *              The Eddystone-UID instance ID.
     */
    UIDFrame(const UIDNamespaceID_t uidNamespaceIDIn, const UIDInstanceID_t  uidInstanceIDIn);

    /**
     * Set the instance and namespace ID.
     *
     * @param[in] uidNamespaceIDIn
     *              The new Eddystone-UID namespace ID.
     * @param[in] uidInstanceIDIn
     *              The new Eddystone-UID instance ID.
     */
    void setUIDData(const UIDNamespaceID_t &uidNamespaceIDIn, const UIDInstanceID_t &uidInstanceIDIn);

    /**
     * Construct the raw bytes of the Eddystone-UID frame that will be directly
     * used in the advertising packets.
     *
     * @param[in] rawFrame
     *              Pointer to the location where the raw frame will be stored.
     * @param[in] advPowerLevel
     *              Power level value included withing the raw frame.
     */
    void constructUIDFrame(uint8_t *rawFrame, int8_t advPowerLevel);

    /**
     * Get the size of the Eddystone-UID frame constructed with the
     * current state of the UIDFrame object.
     *
     * @return The size in bytes of the Eddystone-UID frame.
     */
    size_t getRawFrameSize(void) const;

    /**
     * Get the Eddystone-UID namespace ID.
     *
     * @return A pointer to the namespace ID.
     */
    uint8_t* getUIDNamespaceID(void);

    /**
     * Get the Eddystone-UID instance ID.
     *
     * @return A pointer to the instance ID.
     */
    uint8_t* getUIDInstanceID(void);

private:
    /**
     *  The byte ID of an Eddystone-UID frame.
     */
    static const uint8_t FRAME_TYPE_UID = 0x00;
    /**
     * The size (in bytes) of an Eddystone-UID frame.
     */
    static const uint8_t FRAME_SIZE_UID = 20;

    /**
     * The Eddystone-UID namespace ID.
     */
    UIDNamespaceID_t     uidNamespaceID;
    /**
     * The Eddystone-UID instance ID.
     */
    UIDInstanceID_t      uidInstanceID;
};

#endif  /* __UIDFRAME_H__ */
