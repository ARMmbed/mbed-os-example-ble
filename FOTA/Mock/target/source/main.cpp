/*
 * Mbed-OS Microcontroller Library
 * Copyright (c) 2020-2021 Embedded Planet
 * Copyright (c) 2020-2021 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
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
 * limitations under the License
 */

#include "ble/BLE.h"
#include "ble/gap/Gap.h"
#include "mbedtls/sha256.h"
#include "mbedtls/platform.h"
#include "blockdevice/SlicingBlockDevice.h"
#include "ble-service-fota/FOTAService.h"
#include "BlockDeviceFOTAEventHandler.h"
#include "mbed-trace/mbed_trace.h"

#define TRACE_GROUP "MAIN"

#define SLOT_SIZE 524288

const static char DEVICE_NAME[] = "FOTA";

static events::EventQueue event_queue(/* event count */ 10 * EVENTS_EVENT_SIZE);
static ChainableGapEventHandler chainable_gap_event_handler;
static ChainableGattServerEventHandler chainable_gatt_server_event_handler;

/*
 * Update BlockDevice hook
 *
 * This hook returns a pointer to a SlicingBlockDevice of size = 0.5 MiB =
 * SLOT_SIZE. The SlicingBlockingDevice is backed by the default instance. The
 * nature of this device depends on the targets default configuration in mbed-
 * os/targets/targets.json. However, it must be SPIF, QSPIF or OSPIF. For example,
 * the NRF52840 MCU has QSPIF in its "components_add" list, so the default
 * instance is QSPIFBlockDevice.
 */
mbed::BlockDevice* get_secondary_bd()
{
    mbed::BlockDevice* default_bd = mbed::BlockDevice::get_default_instance();
    static mbed::SlicingBlockDevice sliced_bd(default_bd, 0x0, SLOT_SIZE);
    return &sliced_bd;
}

static void print_hex(const char *title, const unsigned char buf[], size_t len)
{
    mbedtls_printf("%s: ", title);

    for (size_t i = 0; i < len; i++)
        mbedtls_printf("%02x", buf[i]);

    mbedtls_printf("\r\n");
}

/*
 * Event handler for FOTA events
 *
 * For requests other than commit, the handler delegates responsibility to the
 * base class. On commit, the update is "mocked" by computing the SHA-256 of the
 * binary and writing it to the serial.
 */
class FOTADemoEventHandler : public BlockDeviceFOTAEventHandler {
public:
    FOTADemoEventHandler(mbed::BlockDevice &bd, events::EventQueue &eq, BLE &ble) :
            BlockDeviceFOTAEventHandler(bd, eq),
            _ble(ble)
    {
    }

    GattAuthCallbackReply_t on_control_written(FOTAService &fota_service,
                                               mbed::Span<const uint8_t> buffer) override
    {
        /* Check for FOTA Commit request */
        if (buffer[0] == FOTAService::FOTA_COMMIT) {
            tr_info("Committing the update");

            static constexpr const size_t FRAGMENT_SIZE = 128;

            int error = MBEDTLS_EXIT_FAILURE;
            if((error = mbedtls_platform_setup(nullptr)) != 0) {
                tr_error("Platform initialization failed with error %d\r\n", error);
                return AUTH_CALLBACK_REPLY_ATTERR_UNLIKELY_ERROR;
            }

#if defined(MBEDTLS_USE_PSA_CRYPTO)
            /*
             * Initialize underlying PSA Crypto implementation.
             * Even if the HTTPS client doesn't make use of
             * PSA-specific API, for example for setting opaque PSKs
             * or opaque private keys, Mbed TLS will use PSA
             * for public and symmetric key operations as well as
             * hashing.
             */
            psa_status_t status;
            status = psa_crypto_init();
            if( status != PSA_SUCCESS )
            {
                printf("psa_crypto_init() failed with %d\r\n", status );
                return MBEDTLS_EXIT_FAILURE;
            }
#endif /* MBEDTLS_USE_PSA_CRYPTO */

            uint8_t buf[FRAGMENT_SIZE];
            bd_addr_t addr = 0;

            mbedtls_sha256_context ctx;
            mbedtls_sha256_init( &ctx);

            mbedtls_sha256_starts_ret(&ctx, /*is224=*/0);

            size_t size = FRAGMENT_SIZE;
            while (addr < _addr) {
                error = _bd.read(buf, addr, size);
                if (error) {
                    tr_error("Reading block device failed: %d", error);
                }

                mbedtls_sha256_update_ret(&ctx, buf, size);

                addr += size;
                if (_addr - addr < size) {
                    size = _addr - addr;
                }
            }

            uint8_t hash[32];
            mbedtls_sha256_finish(&ctx, hash);

            mbedtls_sha256_free(&ctx);

            print_hex("hash", hash, sizeof hash);

            mbedtls_platform_teardown(nullptr);

            _bd.deinit();

            disconnect(ble::local_disconnection_reason_t::USER_TERMINATION);

            return AUTH_CALLBACK_REPLY_SUCCESS;
        } else {
            /* Let the base class handle the other requests */
            return BlockDeviceFOTAEventHandler::on_control_written(fota_service, buffer);
        }
    }

    void disconnect(ble::local_disconnection_reason_t disconnection_reason)
    {
        _ble.gap().disconnect(_connection_handle, disconnection_reason);
    }

    void set_connection_handle(ble::connection_handle_t connection_handle)
    {
        _connection_handle = connection_handle;
    }

private:
    BLE &_ble;

    ble::connection_handle_t _connection_handle = 0;
};

class FOTAServiceDemo : public ble::Gap::EventHandler,
                        public FOTAService::EventHandler {
public:
    FOTAServiceDemo(BLE &ble,
                    events::EventQueue &eq,
                    ChainableGapEventHandler &chainable_gap_eh,
                    ChainableGattServerEventHandler &chainable_gatt_server_eh) :
                    _ble(ble),
                    _event_queue(eq),
                    _chainable_gap_event_handler(chainable_gap_eh),
                    _chainable_gatt_server_event_handler(chainable_gatt_server_eh),
                    _fota_handler(*get_secondary_bd(), eq, ble),
                    _fota_service(_ble,
                                  _event_queue,
                                  _chainable_gap_event_handler,
                                  _chainable_gatt_server_event_handler,
                                  "1.0.0",
                                  "1.0.0",
                                  "Demo"),
                    _adv_data_builder(_adv_buffer)
    {
    }

    void start()
    {
        _ble.init(this, &FOTAServiceDemo::on_init_complete);

        _event_queue.dispatch_forever();
    }

private:
    void on_init_complete(BLE::InitializationCompleteCallbackContext *params)
    {
        if (params->error != BLE_ERROR_NONE) {
            tr_error("BLE initialization failed.");
            return;
        }

        _chainable_gap_event_handler.addEventHandler(this);
        _ble.gap().setEventHandler(&_chainable_gap_event_handler);

        _ble.gattServer().setEventHandler(&_chainable_gatt_server_event_handler);

        _fota_service.init();

        _fota_service.set_event_handler(&_fota_handler);

        start_advertising();
    }

    void start_advertising()
    {
        ble::AdvertisingParameters adv_parameters(
                ble::advertising_type_t::CONNECTABLE_UNDIRECTED,
                ble::adv_interval_t(ble::millisecond_t(100))
        );

        _adv_data_builder.setFlags();
        _adv_data_builder.setAppearance(ble::adv_data_appearance_t::UNKNOWN);
        _adv_data_builder.setName(DEVICE_NAME);

        ble_error_t error = _ble.gap().setAdvertisingParameters(
                ble::LEGACY_ADVERTISING_HANDLE,
                adv_parameters
        );

        if (error) {
            tr_error("_ble.gap().setAdvertisingParameters() failed");
            return;
        }

        error = _ble.gap().setAdvertisingPayload(
                ble::LEGACY_ADVERTISING_HANDLE,
                _adv_data_builder.getAdvertisingData()
        );

        if (error) {
            tr_error("_ble.gap().setAdvertisingPayload() failed");
            return;
        }

        error = _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);

        if (error) {
            tr_error("_ble.gap().startAdvertising() failed");
            return;
        }

        tr_info("Device advertising, please connect");
    }

private:
    void onConnectionComplete(const ble::ConnectionCompleteEvent &event) override
    {
        if (event.getStatus() == ble_error_t::BLE_ERROR_NONE) {
            _fota_handler.set_connection_handle(event.getConnectionHandle());
            tr_info("Client connected, you may now subscribe to updates");
        }
    }

    void onDisconnectionComplete(const ble::DisconnectionCompleteEvent &event) override
    {
        tr_info("Client disconnected, restarting advertising");

        ble_error_t error = _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);

        if (error) {
            tr_error("_ble.gap().startAdvertising() failed");
            return;
        }
    }

private:
    BLE &_ble;
    events::EventQueue &_event_queue;
    ChainableGapEventHandler &_chainable_gap_event_handler;
    ChainableGattServerEventHandler &_chainable_gatt_server_event_handler;

    FOTADemoEventHandler _fota_handler;
    FOTAService _fota_service;

    uint8_t _adv_buffer[ble::LEGACY_ADVERTISING_MAX_SIZE]{};
    ble::AdvertisingDataBuilder _adv_data_builder;
};

void schedule_ble_events(BLE::OnEventsToProcessCallbackContext *context)
{
    event_queue.call(mbed::Callback<void()>(&context->ble, &BLE::processEvents));
}

int main()
{
    mbed_trace_init();
    mbed_trace_include_filters_set("MAIN, FOTA");

    get_secondary_bd()->init();

    BLE &ble = BLE::Instance();
    ble.onEventsToProcess(schedule_ble_events);

    FOTAServiceDemo demo(ble,
                         event_queue,
                         chainable_gap_event_handler,
                         chainable_gatt_server_event_handler);
    demo.start();

    return 0;
}
