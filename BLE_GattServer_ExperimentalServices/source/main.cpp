/* mbed Microcontroller Library
 * Copyright (c) 2020 ARM Limited
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
 * limitations under the License.
 */

#include "ble/BLE.h"
#include "ble/gap/Gap.h"
#include "pretty_printer.h"
#include "LinkLossService.h"

using namespace std::literals::chrono_literals;

const static char DEVICE_NAME[] = "LinkLoss";

static events::EventQueue event_queue(/* event count */ 10 * EVENTS_EVENT_SIZE);
static ChainableGapEventHandler chainable_gap_event_handler;

class LinkLossDemo : ble::Gap::EventHandler, LinkLossService::EventHandler {
public:
    LinkLossDemo(BLE &ble, events::EventQueue &event_queue, ChainableGapEventHandler &chainable_gap_event_handler) :
            _ble(ble),
            _event_queue(event_queue),
            _chainable_gap_event_handler(chainable_gap_event_handler),
            _link_loss_service(_ble, _event_queue, _chainable_gap_event_handler),
            _adv_data_builder(_adv_buffer)
    {
    }

    void start()
    {
        _ble.init(this, &LinkLossDemo::on_init_complete);

        _event_queue.dispatch_forever();
    }

private:
    void on_init_complete(BLE::InitializationCompleteCallbackContext *params)
    {
        if (params->error != BLE_ERROR_NONE) {
            printf("Ble initialization failed.");
            return;
        }

        print_mac_address();

        /* The ChainableGapEventHandler allows us to dispatch events from GAP to more than a single event handler */
        _chainable_gap_event_handler.addEventHandler(this);
        _ble.gap().setEventHandler(&_chainable_gap_event_handler);

        _link_loss_service.init();

        _link_loss_service.set_event_handler(this);
        _link_loss_service.set_alert_timeout(60000ms);

        start_advertising();
    }

    void start_advertising()
    {
        UUID link_loss_service_uuid = GattService::UUID_LINK_LOSS_SERVICE;

        ble::AdvertisingParameters adv_parameters(
                ble::advertising_type_t::CONNECTABLE_UNDIRECTED,
                ble::adv_interval_t(ble::millisecond_t(100))
        );

        _adv_data_builder.setFlags();
        _adv_data_builder.setAppearance(ble::adv_data_appearance_t::UNKNOWN);
        _adv_data_builder.setLocalServiceList({&link_loss_service_uuid, 1});
        _adv_data_builder.setName(DEVICE_NAME);

        ble_error_t error = _ble.gap().setAdvertisingParameters(
                ble::LEGACY_ADVERTISING_HANDLE,
                adv_parameters
        );

        if (error) {
            printf("_ble.gap().setAdvertisingParameters() failed\r\n");
            return;
        }

        error = _ble.gap().setAdvertisingPayload(
                ble::LEGACY_ADVERTISING_HANDLE,
                _adv_data_builder.getAdvertisingData()
        );

        if (error) {
            printf("_ble.gap().setAdvertisingPayload() failed\r\n");
            return;
        }

        error = _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);

        if (error) {
            printf("_ble.gap().startAdvertising() failed\r\n");
            return;
        }

        printf("Device advertising, please connect\r\n");
    }

    void on_alert_requested(LinkLossService::AlertLevel level) final
    {
        if (level == LinkLossService::AlertLevel::MILD_ALERT) {
            printf("Mild Alert!\r\n");
        } else {
            printf("High Alert!\r\n");
        }
    }

    void on_alert_end() final
    {
        printf("Alert ended\r\n");
    }

private:
    void onConnectionComplete(const ble::ConnectionCompleteEvent &event) override
    {
        if (event.getStatus() == ble_error_t::BLE_ERROR_NONE) {
            printf("Client connected, you may now subscribe to updates\r\n");
        }
    }

    void onDisconnectionComplete(const ble::DisconnectionCompleteEvent &event) override
    {
        printf("Client disconnected, restarting advertising\r\n");

        ble_error_t error = _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);

        if (error) {
            printf("_ble.gap().startAdvertising() failed\r\n");
            return;
        }
    }

private:
    BLE &_ble;
    events::EventQueue &_event_queue;
    ChainableGapEventHandler &_chainable_gap_event_handler;

    LinkLossService _link_loss_service;

    uint8_t _adv_buffer[ble::LEGACY_ADVERTISING_MAX_SIZE];
    ble::AdvertisingDataBuilder _adv_data_builder;
};

void schedule_ble_events(BLE::OnEventsToProcessCallbackContext *context)
{
    event_queue.call(Callback<void()>(&context->ble, &BLE::processEvents));
}

int main()
{
    BLE &ble = BLE::Instance();
    ble.onEventsToProcess(schedule_ble_events);

    LinkLossDemo demo(ble, event_queue, chainable_gap_event_handler);
    demo.start();

    return 0;
}
