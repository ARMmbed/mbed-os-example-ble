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

#include <events/mbed_events.h>
#include "ble/BLE.h"
#include "ble/gap/Gap.h"
#include "pretty_printer.h"

using namespace std::literals::chrono_literals;

const static char DEVICE_NAME[] = "SupportedBLEFeatures";

static events::EventQueue event_queue(/* event count */ 16 * EVENTS_EVENT_SIZE);

class SupportedFeatures : ble::Gap::EventHandler {
public:
    SupportedFeatures(BLE &ble, events::EventQueue &event_queue) :
        _ble(ble),
        _event_queue(event_queue),
        _service_uuid(GattService::UUID_DEVICE_INFORMATION_SERVICE),
        _adv_data_builder(_adv_buffer)
    {
    }

    void start()
    {
        _ble.init(this, &SupportedFeatures::on_init_complete);

        _event_queue.dispatch_forever();
    }

private:
    /** Callback triggered when the ble initialization process has finished */
    void on_init_complete(BLE::InitializationCompleteCallbackContext *params)
    {
        if (params->error != BLE_ERROR_NONE) {
            printf("Ble initialization failed.");
            return;
        }

        print_mac_address();

        /* display basic info and optional BLE features supported by our device */

        printf("BLE Version %s\n", _ble.getVersion());
        printf("Conn handle %d\n", ble::connection_handle_t(&_ble));
        printf("LE_ENCRYPTION feature supported %d\n", _ble.gap().isFeatureSupported(ble::controller_supported_features_t::LE_ENCRYPTION));
        printf("CONNECTION_PARAMETERS_REQUEST_PROCEDURE feature supported %d\n", _ble.gap().isFeatureSupported(ble::controller_supported_features_t::CONNECTION_PARAMETERS_REQUEST_PROCEDURE));
        printf("EXTENDED_REJECT_INDICATION feature supported %d\n", _ble.gap().isFeatureSupported(ble::controller_supported_features_t::EXTENDED_REJECT_INDICATION));
        printf("SLAVE_INITIATED_FEATURES_EXCHANGE feature supported %d\n", _ble.gap().isFeatureSupported(ble::controller_supported_features_t::SLAVE_INITIATED_FEATURES_EXCHANGE));
        printf("LE_PING feature supported %d\n", _ble.gap().isFeatureSupported(ble::controller_supported_features_t::LE_PING));
        printf("LE_DATA_PACKET_LENGTH_EXTENSION feature supported %d\n", _ble.gap().isFeatureSupported(ble::controller_supported_features_t::LE_DATA_PACKET_LENGTH_EXTENSION));
        printf("LL_PRIVACY feature supported %d\n", _ble.gap().isFeatureSupported(ble::controller_supported_features_t::LL_PRIVACY));
        printf("EXTENDED_SCANNER_FILTER_POLICIES feature supported %d\n", _ble.gap().isFeatureSupported(ble::controller_supported_features_t::EXTENDED_SCANNER_FILTER_POLICIES));
        printf("LE_2M_PHY feature supported %d\n", _ble.gap().isFeatureSupported(ble::controller_supported_features_t::LE_2M_PHY));
        printf("STABLE_MODULATION_INDEX_TRANSMITTER feature supported %d\n", _ble.gap().isFeatureSupported(ble::controller_supported_features_t::STABLE_MODULATION_INDEX_TRANSMITTER));
        printf("STABLE_MODULATION_INDEX_RECEIVER feature supported %d\n", _ble.gap().isFeatureSupported(ble::controller_supported_features_t::STABLE_MODULATION_INDEX_RECEIVER));
        printf("LE_CODED_PHY feature supported %d\n", _ble.gap().isFeatureSupported(ble::controller_supported_features_t::LE_CODED_PHY));
        printf("LE_EXTENDED_ADVERTISING feature supported %d\n", _ble.gap().isFeatureSupported(ble::controller_supported_features_t::LE_EXTENDED_ADVERTISING));
        printf("LE_PERIODIC_ADVERTISING feature supported %d\n", _ble.gap().isFeatureSupported(ble::controller_supported_features_t::LE_PERIODIC_ADVERTISING));
        printf("CHANNEL_SELECTION_ALGORITHM_2 feature supported %d\n", _ble.gap().isFeatureSupported(ble::controller_supported_features_t::CHANNEL_SELECTION_ALGORITHM_2));
        printf("LE_POWER_CLASS feature supported %d\n", _ble.gap().isFeatureSupported(ble::controller_supported_features_t::LE_POWER_CLASS));

        /* this allows us to capture events like onConnectionComplete() */
        _ble.gap().setEventHandler(this);

        start_advertising();
    }

    void start_advertising()
    {
        /* Configure advertising parameters */

        ble::AdvertisingParameters adv_parameters(
            ble::advertising_type_t::NON_CONNECTABLE_UNDIRECTED,
            ble::adv_interval_t(ble::millisecond_t(100))
        );


        /* Configure advertising payload */

        _adv_data_builder.setFlags();
        _adv_data_builder.setAppearance(ble::adv_data_appearance_t::UNKNOWN);
        _adv_data_builder.setLocalServiceList({&_service_uuid, 1});
        _adv_data_builder.setName(DEVICE_NAME);


        /* Set advertising parameters configuration to our BLE instance */

        ble_error_t error = _ble.gap().setAdvertisingParameters(
            ble::LEGACY_ADVERTISING_HANDLE,
            adv_parameters
        );

        if (error) {
            printf("_ble.gap().setAdvertisingParameters() failed\r\n");
            return;
        }

        /* Set advertising payload configuration to our BLE instance */

        error = _ble.gap().setAdvertisingPayload(
            ble::LEGACY_ADVERTISING_HANDLE,
            _adv_data_builder.getAdvertisingData()
        );

        if (error) {
            printf("_ble.gap().setAdvertisingPayload() failed\r\n");
            return;
        }

        /* Start advertising */

        error = _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);

        if (error) {
            printf("_ble.gap().startAdvertising() failed\r\n");
            return;
        }

        printf("Device is advertising\r\n");

    }

    /* these implement ble::Gap::EventHandler */
private:
    BLE &_ble;
    events::EventQueue &_event_queue;

    UUID _service_uuid;

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

    SupportedFeatures demo(ble, event_queue);
    demo.start();

    return 0;
}
