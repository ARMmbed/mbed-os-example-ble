/* mbed Microcontroller Library
 * Copyright (c) 2021 ARM Limited
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

static events::EventQueue event_queue(/* event count */ 16 * EVENTS_EVENT_SIZE);

class SupportedFeatures : ble::Gap::EventHandler {
public:
    SupportedFeatures(BLE &ble, events::EventQueue &event_queue) :
        _ble(ble),
        _event_queue(event_queue)
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

        /* display basic info and optional BLE features supported by our device */

        printf("\r\nList of optional BLE features that are supported/unsupported by this board:\r\n");
        feature_support("LE_ENCRYPTION", ble::controller_supported_features_t::LE_ENCRYPTION);
        feature_support("CONNECTION_PARAMETERS_REQUEST_PROCEDURE", ble::controller_supported_features_t::CONNECTION_PARAMETERS_REQUEST_PROCEDURE);
        feature_support("EXTENDED_REJECT_INDICATION", ble::controller_supported_features_t::EXTENDED_REJECT_INDICATION);
        feature_support("SLAVE_INITIATED_FEATURES_EXCHANGE", ble::controller_supported_features_t::SLAVE_INITIATED_FEATURES_EXCHANGE);
        feature_support("SLAVE_INITIATED_FEATURES_EXCHANGE", ble::controller_supported_features_t::SLAVE_INITIATED_FEATURES_EXCHANGE);
        feature_support("LE_PING", ble::controller_supported_features_t::LE_PING);
        feature_support("LE_DATA_PACKET_LENGTH_EXTENSION", ble::controller_supported_features_t::LE_DATA_PACKET_LENGTH_EXTENSION);
        feature_support("LL_PRIVACY", ble::controller_supported_features_t::LL_PRIVACY);
        feature_support("EXTENDED_SCANNER_FILTER_POLICIES", ble::controller_supported_features_t::EXTENDED_SCANNER_FILTER_POLICIES);
        feature_support("LE_2M_PHY", ble::controller_supported_features_t::LE_2M_PHY);
        feature_support("STABLE_MODULATION_INDEX_TRANSMITTER", ble::controller_supported_features_t::STABLE_MODULATION_INDEX_TRANSMITTER);
        feature_support("STABLE_MODULATION_INDEX_RECEIVER", ble::controller_supported_features_t::STABLE_MODULATION_INDEX_RECEIVER);
        feature_support("LE_CODED_PHY", ble::controller_supported_features_t::LE_CODED_PHY);
        feature_support("LE_EXTENDED_ADVERTISING", ble::controller_supported_features_t::LE_EXTENDED_ADVERTISING);
        feature_support("LE_PERIODIC_ADVERTISING", ble::controller_supported_features_t::LE_PERIODIC_ADVERTISING);
        feature_support("CHANNEL_SELECTION_ALGORITHM_2", ble::controller_supported_features_t::CHANNEL_SELECTION_ALGORITHM_2);
        feature_support("LE_POWER_CLASS", ble::controller_supported_features_t::LE_POWER_CLASS);
    }

private:
    /* pretty prints feature support */
    void feature_support(const char* feature_name, ble::controller_supported_features_t feature)
    {
        const bool supported = _ble.gap().isFeatureSupported(feature);

        if (supported) {
            printf("+ %s feature supported\r\n", feature_name);
        } else {
            printf("- %s feature not supported\r\n", feature_name);
        }
    }

private:
    BLE &_ble;
    events::EventQueue &_event_queue;
};

/* Schedule processing of events from the BLE middleware in the event queue. */
void schedule_ble_events(BLE::OnEventsToProcessCallbackContext *context)
{
    event_queue.call(mbed::Callback<void()>(&context->ble, &BLE::processEvents));
}

int main()
{
    BLE &ble = BLE::Instance();
    ble.onEventsToProcess(schedule_ble_events);

    SupportedFeatures demo(ble, event_queue);
    demo.start();

    return 0;
}
