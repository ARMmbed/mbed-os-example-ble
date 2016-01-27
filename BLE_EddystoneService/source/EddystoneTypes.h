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

/**
 * Macro to expand a 16-bit Eddystone UUID to 128-bit UUID.
 */
#define UUID_URL_BEACON(FIRST, SECOND) {                         \
        0xee, 0x0c, FIRST, SECOND, 0x87, 0x86, 0x40, 0xba,       \
        0xab, 0x96, 0x99, 0xb9, 0x1a, 0xc9, 0x81, 0xd8,          \
}

/**
 * Eddystone 16-bit UUID.
 */
const uint8_t EDDYSTONE_UUID[] = {0xAA, 0xFE};

/**
 * 128-bit UUID for Eddystone-URL Configuration Service.
 */
const uint8_t UUID_URL_BEACON_SERVICE[]    = UUID_URL_BEACON(0x20, 0x80);
/**
 * 128-bit UUID for Eddystone-URL Configuration Service Lock State
 * characteristic.
 */
const uint8_t UUID_LOCK_STATE_CHAR[]       = UUID_URL_BEACON(0x20, 0x81);
/**
 * 128-bit UUID for Eddystone-URL Configuration Service Lock
 * characteristic.
 */
const uint8_t UUID_LOCK_CHAR[]             = UUID_URL_BEACON(0x20, 0x82);
/**
 * 128-bit UUID for Eddystone-URL Configuration Service Unlock
 * characteristic.
 */
const uint8_t UUID_UNLOCK_CHAR[]           = UUID_URL_BEACON(0x20, 0x83);
/**
 * 128-bit UUID for Eddystone-URL Configuration Service URI Data
 * characteristic.
 */
const uint8_t UUID_URL_DATA_CHAR[]         = UUID_URL_BEACON(0x20, 0x84);
/**
 * 128-bit UUID for Eddystone-URL Configuration Service Flags
 * characteristic.
 */
const uint8_t UUID_FLAGS_CHAR[]            = UUID_URL_BEACON(0x20, 0x85);
/**
 * 128-bit UUID for Eddystone-URL Configuration Service Advertised TX Power
 * Levels characteristic.
 */
const uint8_t UUID_ADV_POWER_LEVELS_CHAR[] = UUID_URL_BEACON(0x20, 0x86);
/**
 * 128-bit UUID for Eddystone-URL Configuration Service TX Power Mode
 * characteristic.
 */
const uint8_t UUID_TX_POWER_MODE_CHAR[]    = UUID_URL_BEACON(0x20, 0x87);
/**
 * 128-bit UUID for Eddystone-URL Configuration Service Beacon Period
 * characteristic.
 */
const uint8_t UUID_BEACON_PERIOD_CHAR[]    = UUID_URL_BEACON(0x20, 0x88);
/**
 * 128-bit UUID for Eddystone-URL Configuration Service Reset
 * characteristic.
 */
const uint8_t UUID_RESET_CHAR[]            = UUID_URL_BEACON(0x20, 0x89);

/**
 * Default name for the BLE Device Name characteristic.
 */
const char DEFAULT_DEVICE_NAME[] = "EDDYSTONE CONFIG";

/**
 * Default URL used  by EddystoneService.
 */
const char DEFAULT_URL[] = "http://www.mbed.com/";

/**
 * Enumeration that defines the Eddystone power levels for the Eddystone-URL
 * Configuration Service TX Power Mode characteristic. Refer to
 * https://github.com/google/eddystone/blob/master/eddystone-url/docs/config-service-spec.md#37-tx-power-mode.
 */
enum PowerModes {
    /**
     * Lowest transmit power mode.
     */
    TX_POWER_MODE_LOWEST,
    /**
     * Low transmit power mode.
     */
    TX_POWER_MODE_LOW,
    /**
     * Medium transmit power mode.
     */
    TX_POWER_MODE_MEDIUM,
    /**
     * Highest transmit power mode.
     */
    TX_POWER_MODE_HIGH,
    /**
     * Total number of power modes.
     */
    NUM_POWER_MODES
};

/**
 * Type for the 128-bit for Eddystone-URL Configuration Service Lock and Unlock
 * characteristic value.
 */
typedef uint8_t Lock_t[16];
/**
 * Type for the 128-bit for Eddystone-URL Configuration Service Advertised TX
 * Power Levels characteristic value.
 */
typedef int8_t PowerLevels_t[NUM_POWER_MODES];

/**
 * Maximum length of an encoded URL for Eddystone.
 */
const uint16_t URL_DATA_MAX = 18;
/**
 * Type for an encoded URL for Eddystone.
 */
typedef uint8_t UrlData_t[URL_DATA_MAX];

/**
 * Size in bytes of UID namespace ID.
 */
const size_t UID_NAMESPACEID_SIZE = 10;
/**
 * Type for the UID namespace ID.
 */
typedef uint8_t UIDNamespaceID_t[UID_NAMESPACEID_SIZE];
/**
 * Size in bytes of UID instance ID.
 */
const size_t UID_INSTANCEID_SIZE = 6;
/**
 * Type for the UID instance ID.
 */
typedef uint8_t UIDInstanceID_t[UID_INSTANCEID_SIZE];

/**
 * Type for callbacks to update Eddystone-TLM frame Batery Voltage and Beacon
 * Temperature.
 */
typedef uint16_t (*TlmUpdateCallback_t) (uint16_t);

/**
 * Size of Eddystone UUID. Needed to construct all frames raw bytes.
 */
const uint16_t EDDYSTONE_UUID_SIZE = sizeof(EDDYSTONE_UUID);

#endif /* __EDDYSTONETYPES_H__ */
