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

#if defined(TARGET_NRF51822) || defined(TARGET_NRF52832) /* Persistent storage supported on nrf51 and nrf52 platforms */

extern "C" {
    #include "fstorage.h"
}

#include "nrf_error.h"
#include "../../EddystoneService.h"
#include <cstddef>

/**
 * Nordic specific structure used to store params persistently.
 * It extends EddystoneService::EddystoneParams_t with a persistence signature.
 */
struct PersistentParams_t {
    EddystoneService::EddystoneParams_t params;
    uint32_t                         persistenceSignature; /* This isn't really a parameter, but having the expected
                                                            * magic value in this field indicates persistence. */

    static const uint32_t MAGIC = 0x1BEAC000;              /* Magic that identifies persistence */
};

/**
 * The following is a module-local variable to hold configuration parameters for
 * short periods during flash access. This is necessary because the fstorage
 * APIs don't copy in the memory provided as data source. The memory cannot be
 * freed or reused by the application until this flash access is complete. The
 * load and store operations in this module initialize persistentParams and then
 * pass it on to the 'fstorage' APIs.
 */
static PersistentParams_t persistentParams;

/**
 * Dummy callback handler needed by Nordic's fstorage module. This is called
 * after every flash access.
 */
static void fs_evt_handler(fs_evt_t const * const evt, fs_ret_t result)
{
    /* Supress compiler warnings */
    (void) evt;
    (void) result;
}

FS_REGISTER_CFG(fs_config_t fs_config) = {
    NULL,              // Begin pointer (set by fs_init)
    NULL,              // End pointer (set by fs_init)
    &fs_evt_handler,   // Function for event callbacks.
    1,                 // Number of physical flash pages required.
    0xFE               // Priority for flash usage.
};


void loadPersistentParams(void) {
    // copy from flash into persistent params struct
    memcpy(&persistentParams, fs_config.p_start_addr, sizeof(PersistentParams_t));
}

/* Platform-specific implementation for persistence on the nRF5x. Based on the
 * fstorage module provided by the Nordic SDK. */
bool loadEddystoneServiceConfigParams(EddystoneService::EddystoneParams_t *paramsP)
{
    static bool fstorageInited = false;
    if (!fstorageInited) {
        fs_init();
        fstorageInited = true;
    }

    loadPersistentParams();

    if ((persistentParams.persistenceSignature != PersistentParams_t::MAGIC)) {
        // On failure zero out and let the service reset to defaults
        memset(paramsP, 0, sizeof(EddystoneService::EddystoneParams_t));
        return false;
    }

    memcpy(paramsP, &persistentParams.params, sizeof(EddystoneService::EddystoneParams_t));
    return true;
}

/* Platform-specific implementation for persistence on the nRF5x. Based on the
 * fstorage module provided by the Nordic SDK. */
void saveEddystoneServiceConfigParams(const EddystoneService::EddystoneParams_t *paramsP)
{
    memcpy(&persistentParams.params, paramsP, sizeof(EddystoneService::EddystoneParams_t));
    if (persistentParams.persistenceSignature != PersistentParams_t::MAGIC) {
        persistentParams.persistenceSignature = PersistentParams_t::MAGIC;
    } else {
        fs_erase(&fs_config, fs_config.p_start_addr, sizeof(PersistentParams_t) / 4);
    }

    fs_store(&fs_config,
             fs_config.p_start_addr,
             reinterpret_cast<uint32_t *>(&persistentParams),
             sizeof(PersistentParams_t) / 4);
}

#endif /* #ifdef TARGET_NRF51822 */
