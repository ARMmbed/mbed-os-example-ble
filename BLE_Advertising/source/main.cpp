/* mbed Microcontroller Library
 * Copyright (c) 2006-2019 ARM Limited
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

#include <events/mbed_events.h>
#include "ble/BLE.h"
#include "ble/Gap.h"
#include "pretty_printer.h"

const static char DEVICE_NAME[] = "BATTERY";

using namespace std::literals::chrono_literals;

static events::EventQueue event_queue(/* event count */ 16 * EVENTS_EVENT_SIZE);

class BatteryDemo : ble::Gap::EventHandler {
public:
    BatteryDemo(BLE &ble, events::EventQueue &event_queue) :
        _ble(ble),
        _event_queue(event_queue),
        _battery_level(50),
        _adv_data_builder(_adv_buffer)
    {
    }

    void start()
    {
        /* mbed will call on_init_complete when when ble is ready */
        _ble.init(this, &BatteryDemo::on_init_complete);

        /* this will never return */
        _event_queue.dispatch_forever();
    }

private:
    /** Callback triggered when the ble initialization process has finished */
    void on_init_complete(BLE::InitializationCompleteCallbackContext *params)
    {
        if (params->error != BLE_ERROR_NONE) {
            print_error(params->error, "Ble initialization failed.");
            return;
        }

        print_mac_address();

        start_advertising();
    }

    void start_advertising()
    {
        /* create advertising parameters and payload */

        ble::AdvertisingParameters adv_parameters(
            /* you cannot connect to this device, you can only read its advertising data,
             * scannable means that the device has extra advertising data that the peer can receive if it
             * "scans" it which means it is using active scanning (it sends a scan request) */
            ble::advertising_type_t::SCANNABLE_UNDIRECTED,
            ble::adv_interval_t(ble::millisecond_t(1000))
        );

        /* when advertising you can optionally add extra data that is only sent
         * if the central requests it by doing active scanning (sending scan requests),
         * in this example we set this payload first because we want to later reuse
         * the same _adv_data_builder builder for payload updates */

        const uint8_t _vendor_specific_data[4] = { 0xAD, 0xDE, 0xBE, 0xEF };
        _adv_data_builder.setManufacturerSpecificData(_vendor_specific_data);

        _ble.gap().setAdvertisingScanResponse(
            ble::LEGACY_ADVERTISING_HANDLE,
            _adv_data_builder.getAdvertisingData()
        );

        /* now we set the advertising payload that gets sent during advertising without any scan requests */

        _adv_data_builder.clear();
        _adv_data_builder.setFlags();
        _adv_data_builder.setName(DEVICE_NAME);

        /* we add the battery level as part of the payload so it's visible to any device that scans,
         * this part of the payload will be updated periodically without affecting the rest of the payload */
        _adv_data_builder.setServiceData(GattService::UUID_BATTERY_SERVICE, {&_battery_level, 1});

        /* setup advertising */

        ble_error_t error = _ble.gap().setAdvertisingParameters(
            ble::LEGACY_ADVERTISING_HANDLE,
            adv_parameters
        );

        if (error) {
            print_error(error, "_ble.gap().setAdvertisingParameters() failed");
            return;
        }

        error = _ble.gap().setAdvertisingPayload(
            ble::LEGACY_ADVERTISING_HANDLE,
            _adv_data_builder.getAdvertisingData()
        );

        if (error) {
            print_error(error, "_ble.gap().setAdvertisingPayload() failed");
            return;
        }

        /* start advertising */

        error = _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);

        if (error) {
            print_error(error, "_ble.gap().startAdvertising() failed");
            return;
        }

        /* we simulate battery discharging by updating it every second */
        _event_queue.call_every(
            1000ms,
            [this]() {
                update_battery_level();
            }
        );
    }

    void update_battery_level()
    {
        if (_battery_level-- == 10) {
            _battery_level = 100;
        }

        /* update the payload with the new value of the bettery level, the rest of the payload remains the same */
        ble_error_t error = _adv_data_builder.setServiceData(GattService::UUID_BATTERY_SERVICE, make_Span(&_battery_level, 1));

        if (error) {
            print_error(error, "_adv_data_builder.setServiceData() failed");
            return;
        }

        /* set the new payload, we don't need to stop advertising */
        error = _ble.gap().setAdvertisingPayload(
            ble::LEGACY_ADVERTISING_HANDLE,
            _adv_data_builder.getAdvertisingData()
        );

        if (error) {
            print_error(error, "_ble.gap().setAdvertisingPayload() failed");
            return;
        }
    }

private:
    BLE &_ble;
    events::EventQueue &_event_queue;

    uint8_t _battery_level;

    uint8_t _adv_buffer[ble::LEGACY_ADVERTISING_MAX_SIZE];
    ble::AdvertisingDataBuilder _adv_data_builder;
};

/* Schedule processing of events from the BLE middleware in the event queue. */
void schedule_ble_events(BLE::OnEventsToProcessCallbackContext *context)
{
    event_queue.call(Callback<void()>(&context->ble, &BLE::processEvents));
}

int main()
{
    BLE &ble = BLE::Instance();
    ble.onEventsToProcess(schedule_ble_events);

    BatteryDemo demo(ble, event_queue);
    demo.start();

    return 0;
}
