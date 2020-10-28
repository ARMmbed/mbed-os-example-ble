/* mbed Microcontroller Library
 * Copyright (c) 2006-2018 ARM Limited
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
#include "mbed-os-ble-utils/pretty_printer.h"

/** This example demonstrates extended and periodic advertising
 */

using namespace std::literals::chrono_literals;

events::EventQueue event_queue;

static const char DEVICE_NAME[] = "Periodic";

static const uint16_t MAX_ADVERTISING_PAYLOAD_SIZE = 50;

/** Demonstrate periodic advertising and scanning and syncing with the advertising
 */
class PeriodicDemo : private mbed::NonCopyable<PeriodicDemo>, public ble::Gap::EventHandler
{
public:
    PeriodicDemo(BLE& ble, events::EventQueue& event_queue) :
        _ble(ble),
        _event_queue(event_queue),
        _adv_data_builder(_adv_buffer)
    {
    }

    ~PeriodicDemo()
    {
        if (_ble.hasInitialized()) {
            _ble.shutdown();
        }
    }

    /** Start BLE interface initialisation */
    void run()
    {
        /* handle gap events */
        _ble.gap().setEventHandler(this);

        ble_error_t error = _ble.init(this, &PeriodicDemo::on_init_complete);
        if (error) {
            print_error(error, "Error returned by BLE::init\r\n");
            return;
        }

        /* this will not return until shutdown */
        _event_queue.dispatch_forever();
    }

private:
    /** This is called when BLE interface is initialised and starts the first mode */
    void on_init_complete(BLE::InitializationCompleteCallbackContext *event)
    {
        if (event->error) {
            print_error(event->error, "Error during the initialisation\r\n");
            return;
        }

        if (!_ble.gap().isFeatureSupported(ble::controller_supported_features_t::LE_EXTENDED_ADVERTISING) ||
            !_ble.gap().isFeatureSupported(ble::controller_supported_features_t::LE_PERIODIC_ADVERTISING)) {
            printf("Periodic advertising not supported, cannot run example.\r\n");
            return;
        }

        print_mac_address();

        /* all calls are serialised on the user thread through the event queue */
        start_role();
    }

    void start_role()
    {
        /* This example is designed to be run on two boards at the same time,
         * depending on our role we will either be the advertiser or scanner,
         * until the roles are established we will cycle the roles until we find each other */
        if (_role_established) {
            if (_is_scanner) {
                _event_queue.call(this, &PeriodicDemo::scan_periodic);
            } else {
                _event_queue.call(this, &PeriodicDemo::advertise_periodic);
            }
        } else {
            _is_scanner = !_is_scanner;

            if (_is_scanner) {
                _event_queue.call(this, &PeriodicDemo::scan);
            } else {
                _event_queue.call(this, &PeriodicDemo::advertise);
            }
        }
    }

    /** Set up and start advertising */
    void advertise()
    {
        ble_error_t error;

        ble::AdvertisingParameters adv_parameters(
            ble::advertising_type_t::CONNECTABLE_NON_SCANNABLE_UNDIRECTED,
            ble::adv_interval_t(ble::millisecond_t(100))
        );

        adv_parameters.setUseLegacyPDU(false);

        error = _ble.gap().createAdvertisingSet(
            &_adv_handle,
            adv_parameters
        );

        if (error) {
            print_error(error, "Gap::createAdvertisingSet() failed\r\n");
            return;
        }

        _adv_data_builder.setFlags();
        _adv_data_builder.setName(DEVICE_NAME);

        /* Set payload for the set */
        error = _ble.gap().setAdvertisingPayload(
            _adv_handle,
            _adv_data_builder.getAdvertisingData()
        );

        if (error) {
            print_error(error, "Gap::setAdvertisingPayload() failed\r\n");
            return;
        }

        /* since we have two boards which might start running this example at the same time
         * we randomise the interval of advertising to have them meet when one is advertising
         * and the other one is scanning (we use their random address as source of randomness) */
        ble::millisecond_t random_duration_ms((2 + rand() % 5) * 1000);
        ble::adv_duration_t random_duration(random_duration_ms);

        error = _ble.gap().startAdvertising(
            _adv_handle,
            random_duration
        );

        if (error) {
            print_error(error, "Gap::startAdvertising() failed\r\n");
            return;
        }

        printf("Advertising started for %dms\r\n", random_duration_ms);
    }

    void advertise_periodic()
    {
        ble::AdvertisingParameters adv_parameters(
            ble::advertising_type_t::NON_CONNECTABLE_UNDIRECTED,
            ble::adv_interval_t(ble::millisecond_t(200))
        );

        adv_parameters.setUseLegacyPDU(false);

        ble_error_t error = _ble.gap().setAdvertisingParameters(_adv_handle, adv_parameters);

        if (error) {
            print_error(error, "Gap::setAdvertisingParameters() failed\r\n");
            return;
        }

        /* Start advertising the set as the advertising needs to be active
         * before we start periodic advertising */
        error = _ble.gap().startAdvertising(_adv_handle);

        if (error) {
            print_error(error, "Gap::startAdvertising() failed\r\n");
            return;
        }

        /* periodic advertising will be enabled when advertising starts */
    }

    /** Set up and start scanning */
    void scan()
    {
        _is_connecting_or_syncing = false;

        ble::ScanParameters scan_params;
        scan_params.setOwnAddressType(ble::own_address_type_t::RANDOM);

        ble_error_t error = _ble.gap().setScanParameters(scan_params);

        if (error) {
            print_error(error, "Error caused by Gap::setScanParameters\r\n");
            return;
        }

        error = _ble.gap().startScan(ble::scan_duration_t(500));

        if (error) {
            print_error(error, "Error caused by Gap::startScan\r\n");
            return;
        }

        printf("Scanning started\r\n");
    }

    void scan_periodic()
    {
        _is_connecting_or_syncing = false;

        ble_error_t error = _ble.gap().startScan();

        if (error) {
            print_error(error, "Error caused by Gap::startScan\r\n");
            return;
        }

        printf("Scanning for periodic advertising started\r\n");
    }

    /* also updates periodic advertising payload */
    void update_sensor_value()
    {
        /* simulate battery level */
        _battery_level--;
        if (_battery_level < 1) {
            _battery_level = 100;
        }

        /* update the level in the payload */
        ble_error_t error = _adv_data_builder.setServiceData(
            GattService::UUID_BATTERY_SERVICE,
            mbed::make_Span(&_battery_level, 1)
        );

        if (error) {
            print_error(error, "AdvertisingDataBuilder::setServiceData() failed\r\n");
            return;
        }

        /* the data in the local host buffer has been updated but now
         * we have to update the data in the controller */
        error = _ble.gap().setPeriodicAdvertisingPayload(
            _adv_handle,
            _adv_data_builder.getAdvertisingData()
        );

        if (error) {
            print_error(error, "Gap::setPeriodicAdvertisingPayload() failed\r\n");
        }
    }

private:
    /* Gap::EventHandler */

    void onAdvertisingStart(const ble::AdvertisingStartEvent &event) override
    {
        /* start periodic advertising only if we're already advertising after roles established */
        if (_role_established) {
            ble_error_t error = _ble.gap().setPeriodicAdvertisingParameters(
                _adv_handle,
                ble::periodic_interval_t(100),
                ble::periodic_interval_t(1000)
            );

            if (error) {
                print_error(error, "Gap::setPeriodicAdvertisingParameters() failed\r\n");
                return;
            }

            error = _ble.gap().startPeriodicAdvertising(_adv_handle);

            if (error) {
                print_error(error, "Gap::startPeriodicAdvertising() failed\r\n");
                return;
            }

            printf("Periodic advertising started\r\n");

            /* tick over our fake battery data, this will also update the advertising payload */
            _event_queue.call_every(1000ms, this, &PeriodicDemo::update_sensor_value);
        }
    }

    /** Look at scan payload to find a peer device and connect to it */
    void onAdvertisingReport(const ble::AdvertisingReportEvent &event) override
    {
        /* don't bother with analysing scan result if we're already connecting */
        if (_is_connecting_or_syncing) {
            return;
        }

        /* if we're looking for periodic advertising don't bother unless it's present */
        if (_role_established && !event.isPeriodicIntervalPresent()) {
            return;
        }

        ble::AdvertisingDataParser adv_parser(event.getPayload());

        /* parse the advertising payload, looking for a discoverable device */
        while (adv_parser.hasNext()) {
            ble::AdvertisingDataParser::element_t field = adv_parser.next();

            /* identify peer by name */
            if (field.type == ble::adv_data_type_t::COMPLETE_LOCAL_NAME &&
                field.value.size() == strlen(DEVICE_NAME) &&
                (memcmp(field.value.data(), DEVICE_NAME, field.value.size()) == 0)) {
                /* if we haven't established our roles connect, otherwise sync with advertising */
                if (_role_established) {
                    printf("We found the peer, syncing with SID %d"
                           " and periodic interval %dms\r\n",
                           event.getSID(), event.getPeriodicInterval().valueInMs());

                    ble_error_t error = _ble.gap().createSync(
                        event.getPeerAddressType(),
                        event.getPeerAddress(),
                        event.getSID(),
                        2,
                        ble::sync_timeout_t(ble::millisecond_t(5000))
                    );

                    if (error) {
                        print_error(error, "Error caused by Gap::createSync\r\n");
                        return;
                    }
                } else {
                    printf("We found the peer, connecting\r\n");

                    ble_error_t error = _ble.gap().connect(
                        event.getPeerAddressType(),
                        event.getPeerAddress(),
                        ble::ConnectionParameters() // use the default connection parameters
                    );

                    if (error) {
                        print_error(error, "Error caused by Gap::connect\r\n");
                        return;
                    }
                }

                /* we may have already scan events waiting to be processed
                 * so we need to remember that we are already connecting
                 * or syncing and ignore them */
                _is_connecting_or_syncing = true;

                return;
            }
        }
    }

    void onScanTimeout(const ble::ScanTimeoutEvent&) override
    {
        if (!_is_connecting_or_syncing) {
            printf("Scanning ended, failed to find peer\r\n");
            start_role();
        }
    }

    /** This is called by Gap to notify the application we connected */
    void onConnectionComplete(const ble::ConnectionCompleteEvent &event) override
    {
        if (event.getStatus() == BLE_ERROR_NONE) {
            printf("Connected to: ");
            print_address(event.getPeerAddress().data());
            printf("Roles established\r\n");
            _role_established = true;

            if (_is_scanner) {
                printf("I will synchronise with periodic advertising\r\n");
                _event_queue.call_in(
                    1000ms,
                    [this, handle = event.getConnectionHandle()] {
                        _ble.gap().disconnect(
                            handle,
                            ble::local_disconnection_reason_t(ble::local_disconnection_reason_t::USER_TERMINATION)
                        );
                    }
                );
            } else {
                printf("I will advertise periodic advertising\r\n");
            }
        } else {
            printf("Failed to connect\r\n");
            start_role();
        }
    }

    /** This is called by Gap to notify the application we disconnected */
    void onDisconnectionComplete(const ble::DisconnectionCompleteEvent &event) override
    {
        printf("Disconnected\r\n");
        start_role();
    }

    /** Called when first advertising packet in periodic advertising is received. */
    void onPeriodicAdvertisingSyncEstablished(const ble::PeriodicAdvertisingSyncEstablishedEvent &event) override
    {
        if (event.getStatus() == BLE_ERROR_NONE) {
            printf("Synced with periodic advertising\r\n");
            _sync_handle = event.getSyncHandle();
        } else {
            printf("Sync with periodic advertising failed\r\n");
        }
    }

    /** Called when a periodic advertising packet is received. */
    void onPeriodicAdvertisingReport(const ble::PeriodicAdvertisingReportEvent &event) override
    {
        ble::AdvertisingDataParser adv_parser(event.getPayload());

        /* parse the advertising payload, looking for a battery level */
        while (adv_parser.hasNext()) {
            ble::AdvertisingDataParser::element_t field = adv_parser.next();

            if (field.type == ble::adv_data_type_t::SERVICE_DATA) {
                GattService::UUID_BATTERY_SERVICE;
                if (*((uint16_t*)field.value.data()) != GattService::UUID_BATTERY_SERVICE) {
                    printf("Unexpected service data\r\n");
                } else {
                    /* battery level is right after the UUID */
                    const uint8_t *battery_level = field.value.data() + sizeof(uint16_t);
                    printf("Peer battery level: %d\r\n", *battery_level);
                }
            }
        }
    }

    /** Called when a periodic advertising sync has been lost. */
    void onPeriodicAdvertisingSyncLoss(const ble::PeriodicAdvertisingSyncLoss &event) override
    {
        printf("Sync to periodic advertising lost\r\n");
        _sync_handle = ble::INVALID_ADVERTISING_HANDLE;
        _event_queue.call(this, &PeriodicDemo::scan_periodic);
    }

private:
    BLE  &_ble;
    events::EventQueue &_event_queue;

    uint8_t _adv_buffer[MAX_ADVERTISING_PAYLOAD_SIZE];
    ble::AdvertisingDataBuilder _adv_data_builder;

    ble::advertising_handle_t _adv_handle = ble::INVALID_ADVERTISING_HANDLE;
    ble::periodic_sync_handle_t _sync_handle = ble::INVALID_ADVERTISING_HANDLE;

    uint8_t _battery_level = 100;

    bool _is_scanner = false;
    bool _is_connecting_or_syncing = false;
    bool _role_established = false;
};

/** Schedule processing of events from the BLE middleware in the event queue. */
void schedule_ble_events(BLE::OnEventsToProcessCallbackContext *context)
{
    event_queue.call(Callback<void()>(&context->ble, &BLE::processEvents));
}

int main()
{
    BLE &ble = BLE::Instance();

    /* this will inform us off all events so we can schedule their handling
     * using our event queue */
    ble.onEventsToProcess(schedule_ble_events);

    /* look for other device and then settle on a role and sync periodic advertising */
    PeriodicDemo demo(ble, event_queue);

    demo.run();

    return 0;
}
