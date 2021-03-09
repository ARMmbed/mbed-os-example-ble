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

        print_mac_address();

        /* display basic info and optional BLE features supported by our device */

        printf("--- List of optional BLE features that are supported/unsupported by this board ---\n");
        printf("LE_ENCRYPTION feature: %s\n", _ble.gap().isFeatureSupported(ble::controller_supported_features_t::LE_ENCRYPTION) ? "supported" : "not supported");
        printf("CONNECTION_PARAMETERS_REQUEST_PROCEDURE feature: %s\n", _ble.gap().isFeatureSupported(ble::controller_supported_features_t::CONNECTION_PARAMETERS_REQUEST_PROCEDURE) ? "supported" : "not supported");
        printf("EXTENDED_REJECT_INDICATION feature: %s\n", _ble.gap().isFeatureSupported(ble::controller_supported_features_t::EXTENDED_REJECT_INDICATION) ? "supported" : "not supported");
        printf("SLAVE_INITIATED_FEATURES_EXCHANGE feature: %s\n", _ble.gap().isFeatureSupported(ble::controller_supported_features_t::SLAVE_INITIATED_FEATURES_EXCHANGE) ? "supported" : "not supported");
        printf("LE_PING feature supported: %s\n", _ble.gap().isFeatureSupported(ble::controller_supported_features_t::LE_PING) ? "supported" : "not supported");
        printf("LE_DATA_PACKET_LENGTH_EXTENSION feature: %s\n", _ble.gap().isFeatureSupported(ble::controller_supported_features_t::LE_DATA_PACKET_LENGTH_EXTENSION) ? "supported" : "not supported");
        printf("LL_PRIVACY feature: %s\n", _ble.gap().isFeatureSupported(ble::controller_supported_features_t::LL_PRIVACY) ? "supported" : "not supported");
        printf("EXTENDED_SCANNER_FILTER_POLICIES feature: %s\n", _ble.gap().isFeatureSupported(ble::controller_supported_features_t::EXTENDED_SCANNER_FILTER_POLICIES) ? "supported" : "not supported");
        printf("LE_2M_PHY feature supported: %s\n", _ble.gap().isFeatureSupported(ble::controller_supported_features_t::LE_2M_PHY) ? "supported" : "not supported");
        printf("STABLE_MODULATION_INDEX_TRANSMITTER feature: %s\n", _ble.gap().isFeatureSupported(ble::controller_supported_features_t::STABLE_MODULATION_INDEX_TRANSMITTER) ? "supported" : "not supported");
        printf("STABLE_MODULATION_INDEX_RECEIVER feature: %s\n", _ble.gap().isFeatureSupported(ble::controller_supported_features_t::STABLE_MODULATION_INDEX_RECEIVER) ? "supported" : "not supported");
        printf("LE_CODED_PHY feature: %s\n", _ble.gap().isFeatureSupported(ble::controller_supported_features_t::LE_CODED_PHY) ? "supported" : "not supported");
        printf("LE_EXTENDED_ADVERTISING feature: %s\n", _ble.gap().isFeatureSupported(ble::controller_supported_features_t::LE_EXTENDED_ADVERTISING) ? "supported" : "not supported");
        printf("LE_PERIODIC_ADVERTISING feature: %s\n", _ble.gap().isFeatureSupported(ble::controller_supported_features_t::LE_PERIODIC_ADVERTISING) ? "supported" : "not supported");
        printf("CHANNEL_SELECTION_ALGORITHM_2 feature: %s\n", _ble.gap().isFeatureSupported(ble::controller_supported_features_t::CHANNEL_SELECTION_ALGORITHM_2) ? "supported" : "not supported");
        printf("LE_POWER_CLASS feature: %s\n", _ble.gap().isFeatureSupported(ble::controller_supported_features_t::LE_POWER_CLASS) ? "supported" : "not supported");
    }


private:
    BLE &_ble;
    events::EventQueue &_event_queue;

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