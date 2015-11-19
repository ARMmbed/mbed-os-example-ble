
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

#ifndef __EDDYSTONETYPES_H__
#define __EDDYSTONETYPES_H__

#include <stdint.h>
#include <stddef.h>

#define UUID_URL_BEACON(FIRST, SECOND) {                         \
        0xee, 0x0c, FIRST, SECOND, 0x87, 0x86, 0x40, 0xba,       \
        0xab, 0x96, 0x99, 0xb9, 0x1a, 0xc9, 0x81, 0xd8,          \
}

const uint8_t EDDYSTONE_UUID[] = {0xAA, 0xFE};

const uint8_t UUID_URL_BEACON_SERVICE[]    = UUID_URL_BEACON(0x20, 0x80);
const uint8_t UUID_LOCK_STATE_CHAR[]       = UUID_URL_BEACON(0x20, 0x81);
const uint8_t UUID_LOCK_CHAR[]             = UUID_URL_BEACON(0x20, 0x82);
const uint8_t UUID_UNLOCK_CHAR[]           = UUID_URL_BEACON(0x20, 0x83);
const uint8_t UUID_URL_DATA_CHAR[]         = UUID_URL_BEACON(0x20, 0x84);
const uint8_t UUID_FLAGS_CHAR[]            = UUID_URL_BEACON(0x20, 0x85);
const uint8_t UUID_ADV_POWER_LEVELS_CHAR[] = UUID_URL_BEACON(0x20, 0x86);
const uint8_t UUID_TX_POWER_MODE_CHAR[]    = UUID_URL_BEACON(0x20, 0x87);
const uint8_t UUID_BEACON_PERIOD_CHAR[]    = UUID_URL_BEACON(0x20, 0x88);
const uint8_t UUID_RESET_CHAR[]            = UUID_URL_BEACON(0x20, 0x89);

const char DEVICE_NAME[] = "EDDYSTONE CONFIG";

const char DEFAULT_URL[] = "http://www.mbed.com/";

enum FrameType {
    EDDYSTONE_FRAME_URL,
    EDDYSTONE_FRAME_UID,
    EDDYSTONE_FRAME_TLM,
    NUM_EDDYSTONE_FRAMES
};

enum PowerModes {
    TX_POWER_MODE_LOWEST,
    TX_POWER_MODE_LOW,
    TX_POWER_MODE_MEDIUM,
    TX_POWER_MODE_HIGH,
    NUM_POWER_MODES
};

/* 128 bits of lock */
typedef uint8_t Lock_t[16];
typedef int8_t PowerLevels_t[NUM_POWER_MODES];

const uint16_t URL_DATA_MAX = 18;
typedef uint8_t UrlData_t[URL_DATA_MAX];

/* UID Frame Type subfields */
const size_t UID_NAMESPACEID_SIZE = 10;
typedef uint8_t UIDNamespaceID_t[UID_NAMESPACEID_SIZE];
const size_t UID_INSTANCEID_SIZE = 6;
typedef uint8_t UIDInstanceID_t[UID_INSTANCEID_SIZE];

/* Callbacks for updating BateryVoltage and Temperature */
typedef uint16_t (*TlmUpdateCallback_t) (uint16_t);

/* Size of Eddystone UUID needed for all frames */
const uint16_t EDDYSTONE_UUID_SIZE = sizeof(EDDYSTONE_UUID);

#endif /* __EDDYSTONETYPES_H__ */
