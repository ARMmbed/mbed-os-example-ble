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

#include "ble-service-fota/FOTAService.h"

#include "blockdevice/SlicingBlockDevice.h"
#include "BlockDeviceFOTAEventHandler.h"

#include "mbed-trace/mbed_trace.h"

#define TRACE_GROUP "MAIN"

const static char DEVICE_NAME[] = "FOTA";

static events::EventQueue event_queue(/* event count */ 10 * EVENTS_EVENT_SIZE);
static ChainableGapEventHandler chainable_gap_event_handler;
static ChainableGattServerEventHandler chainable_gatt_server_event_handler;

mbed::BlockDevice* get_secondary_bd()
{
    mbed::BlockDevice* default_bd = mbed::BlockDevice::get_default_instance();
    static mbed::SlicingBlockDevice sliced_bd(default_bd, 0x00000, 0xC0000);
    return &sliced_bd;
}

class FOTADemoEventHandler : public BlockDeviceFOTAEventHandler {
public:
    FOTADemoEventHandler(mbed::BlockDevice &bd, events::EventQueue &eq) :
            BlockDeviceFOTAEventHandler(bd, eq)
    {
    }
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
                    _fota_handler(*get_secondary_bd(), eq),
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

    void disconnect(ble::local_disconnection_reason_t disconnection_reason)
    {
        _ble.gap().disconnect(_connection_handle, disconnection_reason);
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
            tr_error("_ble.gap().setAdvertisingParameters() failed\r\n");
            return;
        }

        error = _ble.gap().setAdvertisingPayload(
                ble::LEGACY_ADVERTISING_HANDLE,
                _adv_data_builder.getAdvertisingData()
        );

        if (error) {
            tr_error("_ble.gap().setAdvertisingPayload() failed\r\n");
            return;
        }

        error = _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);

        if (error) {
            tr_error("_ble.gap().startAdvertising() failed\r\n");
            return;
        }

        tr_info("Device advertising, please connect\r\n");
    }

private:
    void onConnectionComplete(const ble::ConnectionCompleteEvent &event) override
    {
        if (event.getStatus() == ble_error_t::BLE_ERROR_NONE) {
            _connection_handle = event.getConnectionHandle();
            tr_info("Client connected, you may now subscribe to updates\r\n");
        }
    }

    void onDisconnectionComplete(const ble::DisconnectionCompleteEvent &event) override
    {
        printf("Client disconnected, restarting advertising\r\n");

        ble_error_t error = _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);

        if (error) {
            tr_error("_ble.gap().startAdvertising() failed\r\n");
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

    ble::connection_handle_t _connection_handle = 0;
};

void schedule_ble_events(BLE::OnEventsToProcessCallbackContext *context)
{
    event_queue.call(mbed::Callback<void()>(&context->ble, &BLE::processEvents));
}

int main()
{
    mbed_trace_init();
    mbed_trace_exclude_filters_set("QSPIF");

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
