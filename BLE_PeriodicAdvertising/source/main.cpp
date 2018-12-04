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
#include <mbed.h>
#include "ble/BLE.h"
#include "gap/Gap.h"
#include "gap/AdvertisingDataParser.h"
#include "pretty_printer.h"
#include "BatteryService.h"

/** This example demonstrates all the basic setup required
 *  to advertise, scan and connect to other devices.
 *
 *  It contains a single class that performs both scans and advertisements.
 */

events::EventQueue event_queue;

static const char DEVICE_NAME[] = "Periodic";

static const uint16_t MAX_ADVERTISING_PAYLOAD_SIZE = 50;
static const uint8_t SCAN_TIME = 5;
static const uint8_t CONNECTION_DURATION = 2;

/** Demonstrate periodic advertising and scanning and syncing with the advertising
 */
class PeriodicDemo : private mbed::NonCopyable<PeriodicDemo>, public ble::Gap::EventHandler
{
public:
    PeriodicDemo(BLE& ble, events::EventQueue& event_queue) :
        _ble(ble),
        _gap(ble.gap()),
        _event_queue(event_queue),
        _led1(LED1, 0),
        _is_scanner(false),
        _is_connecting(false),
        _is_syncing(false),
        _role_established(false),
        _battery_uuid(GattService::UUID_BATTERY_SERVICE),
        _battery_level(100),
        _battery_service(ble, _battery_level),
        _adv_data_builder(_adv_buffer),
        _adv_handle(ble::INVALID_ADVERTISING_HANDLE),
        _sync_handle(ble::INVALID_ADVERTISING_HANDLE)
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
        if (_ble.hasInitialized()) {
            printf("Ble instance already initialised.\r\n");
            return;
        }

        /* handle gap events */
        _gap.setEventHandler(this);

        ble_error_t error = _ble.init(this, &PeriodicDemo::on_init_complete);
        if (error) {
            print_error(error, "Error returned by BLE::init");
            return;
        }

        /* to show we're running we'll blink every 500ms */
        _event_queue.call_every(500, this, &PeriodicDemo::blink);

        /* this will not return until shutdown */
        _event_queue.dispatch_forever();
    }

private:
    /** This is called when BLE interface is initialised and starts the first mode */
    void on_init_complete(BLE::InitializationCompleteCallbackContext *event)
    {
        if (event->error) {
            print_error(event->error, "Error during the initialisation");
            return;
        }

        if (!_gap.isFeatureSupported(ble::controller_supported_features_t::LE_EXTENDED_ADVERTISING) ||
            !_gap.isFeatureSupported(ble::controller_supported_features_t::LE_PERIODIC_ADVERTISING)) {
            printf("Periodic advertising not supported, cannot run example.");
            return;
        }

        print_mac_address();

        /* all calls are serialised on the user thread through the event queue */
        _event_queue.call(this, &PeriodicDemo::start_role);
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
        ble::AdvertisingParameters adv_params;
        adv_params.setUseLegacyPDU(false);

        /* create the advertising set with its parameter */
        ble_error_t error = _gap.createAdvertisingSet(
            &_adv_handle,
            adv_params
        );

        if (error) {
            print_error(error, "Gap::createAdvertisingSet() failed");
            return;
        }

        _adv_data_builder.setFlags();
        _adv_data_builder.setName(DEVICE_NAME);

        /* Set payload for the set */
        error = _gap.setAdvertisingPayload(
            _adv_handle,
            _adv_data_builder.getAdvertisingData()
        );

        if (error) {
            print_error(error, "Gap::setAdvertisingPayload() failed");
            return;
        }

        /* since we have two boards which might start running this example at the same time
         * we randomise the interval of advertising to have them meet when one is advertising
         * and the other one is scanning (we use their random address as source of randomness) */
        ble::millisecond_t random_duration_ms((1 + rand() % 5) * 1000);
        ble::adv_duration_t random_duration(random_duration_ms);

        error = _ble.gap().startAdvertising(
            _adv_handle,
            random_duration
        );

        if (error) {
            print_error(error, "Gap::startAdvertising() failed");
            return;
        }

        printf("Advertising started\r\n");
    }

    void advertise_periodic()
    {

        /* Start advertising the set as the advertising needs to be active
         * before we start periodic advertising */
        ble_error_t error = _gap.startAdvertising(_adv_handle);

        if (error) {
            print_error(error, "Gap::startAdvertising() failed");
            return;
        }

        error = _gap.setPeriodicAdvertisingParameters(
            _adv_handle,
            ble::periodic_interval_t(50),
            ble::periodic_interval_t(500)
        );

        if (error) {
            print_error(error, "Gap::setPeriodicAdvertisingParameters() failed");
            return;
        }

        /* we will put the battery level data in there and update it every second */
        update_paylod();

        error = _gap.startPeriodicAdvertising(_adv_handle);

        if (error) {
            print_error(error, "Gap::startPeriodicAdvertising() failed");
            return;
        }

        printf("Periodic advertising started\r\n");

        /* tick over our fake battery data, this will also update the advertising payload */
        _event_queue.call_every(1000, this, &PeriodicDemo::update_sensor_value);
    }

    void update_paylod()
    {
        /* advertising payload will have the battery level which we will update */
        ble_error_t error = _adv_data_builder.setServiceData(
            _battery_uuid,
            mbed::make_Span(&_battery_level, 1)
        );

        if (error) {
            print_error(error, "AdvertisingDataBuilder::setFlags() failed");
            return;
        }

        /* the data in the local host buffer has been updated but now
         * we have to update the data in the controller */
        error = _gap.setPeriodicAdvertisingPayload(
            _adv_handle,
            _adv_data_builder.getAdvertisingData()
        );

        if (error) {
            print_error(error, "Gap::setPeriodicAdvertisingPayload() failed");
            return;
        }
    }

    /** Set up and start scanning */
    void scan()
    {
        /* keep track of our state */
        _is_connecting = false;

        ble_error_t error = _gap.setScanParameters(
            ble::ScanParameters()
        );

        if (error) {
            print_error(error, "Error caused by Gap::setScanParameters");
            return;
        }

        error = _gap.startScan(
            ble::scan_duration_t(ble::millisecond_t(SCAN_TIME))
        );

        if (error) {
            print_error(error, "Error caused by Gap::startScan");
            return;
        }
    }

    void scan_periodic()
    {
        ble_error_t error = _gap.startScan();

        if (error) {
            print_error(error, "Error caused by Gap::startScan");
            return;
        }

        _is_syncing = false;
    }

private:
    /* Gap::EventHandler */

    /** Look at scan payload to find a peer device and connect to it */
    virtual void onAdvertisingReport(
        const ble::AdvertisingReportEvent &event
    ) {
        if (_role_established) {
            if (_is_syncing) {
                return;
            }
        } else {
            /* don't bother with analysing scan result if we're already connecting */
            if (_is_connecting) {
                return;
            }

            ble::AdvertisingDataParser adv_parser(event.getPayload());

            /* parse the advertising payload, looking for a discoverable device */
            while (adv_parser.hasNext()) {
                ble::AdvertisingDataParser::element_t field = adv_parser.next();

                /* connect by name */
                if (field.type == ble::adv_data_type_t::COMPLETE_LOCAL_NAME &&
                    field.value.size() == sizeof(DEVICE_NAME) &&
                    (memcmp(field.value.data(), DEVICE_NAME, sizeof(DEVICE_NAME)) == 0)) {

                    printf("We found the peer, connecting\r\n");

                    ble_error_t error = _gap.connect(
                        event.getPeerAddressType(),
                        event.getPeerAddress(),
                        ble::ConnectionParameters() // use the default connection parameters
                    );

                    if (error) {
                        print_error(error, "Error caused by Gap::connect\r\n");
                        return;
                    }

                    /* we may have already scan events waiting
                     * to be processed so we need to remember
                     * that we are already connecting and ignore them */
                    _is_connecting = true;
                    return;
                }
            }
        }
    }

    virtual void onAdvertisingEnd(
        const ble::AdvertisingEndEvent &event
    ) {
        if (event.isConnected()) {
            printf("Stopped advertising due to connection\r\n");
        } else {
            _event_queue.call(this, &PeriodicDemo::start_role);
        }
    }

    virtual void onScanTimeout(
        const ble::ScanTimeoutEvent&
    ) {
        printf("Scanning ended, failed to find peer\r\n");
        _event_queue.call(this, &PeriodicDemo::start_role);
    }

    /** This is called by Gap to notify the application we connected */
    virtual void onConnectionComplete(
        const ble::ConnectionCompleteEvent &event
    ) {
        if (event.getStatus() == BLE_ERROR_NONE) {
            printf("Roles established\r\n");
            _role_established = true;

            /* we have to specify the disconnect call because of ambiguous overloads */
            typedef ble_error_t (Gap::*disconnect_call_t)(ble::connection_handle_t, ble::local_disconnection_reason_t);
            const disconnect_call_t disconnect_call = &Gap::disconnect;

            if (_is_scanner) {
                _event_queue.call_in(
                    2000,
                    &_ble.gap(),
                    disconnect_call,
                    event.getConnectionHandle(),
                    ble::local_disconnection_reason_t(ble::local_disconnection_reason_t::USER_TERMINATION)
                );
            }
        } else {
            printf("Failed to connect\r\n");
            _event_queue.call(this, &PeriodicDemo::start_role);
        }
    }

    /** This is called by Gap to notify the application we disconnected */
    virtual void onDisconnectionComplete(
        const ble::DisconnectionCompleteEvent &event
    ) {
        printf("Disconnected\r\n");
        _event_queue.call(this, &PeriodicDemo::start_role);
    }

    /** Called when first advertising packet in periodic advertising is received. */
    virtual void onPeriodicAdvertisingSyncEstablished(
        const ble::PeriodicAdvertisingSyncEstablishedEvent &event
    ) {
        if (event.getStatus() == BLE_ERROR_NONE) {
            printf("Synced with periodic advertising\r\n");
        } else {
            _event_queue.call(this, &PeriodicDemo::scan_periodic);
        }
    }

    /** Called when a periodic advertising packet is received. */
    virtual void onPeriodicAdvertisingReport(
       const ble::PeriodicAdvertisingReportEvent &event
    ) {
        event.getPayload();

        ble::AdvertisingDataParser adv_parser(event.getPayload());

        /* parse the advertising payload, looking for a battery level */
        while (adv_parser.hasNext()) {
            ble::AdvertisingDataParser::element_t field = adv_parser.next();

            if (field.type == ble::adv_data_type_t::SERVICE_DATA) {
                if (memcmp(field.value.data(), _battery_uuid.getBaseUUID(), _battery_uuid.getLen()) != 0) {
                    printf("Unexpected service data\r\n");
                } else {
                    /* battery level is right after the UUID */
                    const uint8_t *battery_level = field.value.data() + _battery_uuid.getLen();
                    printf("Peer battery level: %d\r\n", *battery_level);
                }
            }
        }
    }

    /** Called when a periodic advertising sync has been lost. */
    virtual void onPeriodicAdvertisingSyncLoss(
       const ble::PeriodicAdvertisingSyncLoss &event
    ) {
        _event_queue.call(this, &PeriodicDemo::scan_periodic);
    }

private:
    void update_sensor_value() {
        _battery_level--;
        if (_battery_level < 1) {
            _battery_level = 100;
        }

        _battery_service.updateBatteryLevel(_battery_level);
        update_paylod();
    }

    /** Blink LED to show we're running */
    void blink(void)
    {
        _led1 = !_led1;
    }

private:
    BLE                &_ble;
    ble::Gap           &_gap;
    events::EventQueue &_event_queue;

    DigitalOut _led1;

    bool _is_scanner;
    bool _is_connecting;
    bool _is_syncing;
    bool _role_established;

    UUID            _battery_uuid;
    uint8_t         _battery_level;
    BatteryService  _battery_service;

    uint8_t _adv_buffer[MAX_ADVERTISING_PAYLOAD_SIZE];
    ble::AdvertisingDataBuilder _adv_data_builder;

    ble::advertising_handle_t _adv_handle;
    ble::periodic_sync_handle_t _sync_handle;
};

/** Schedule processing of events from the BLE middleware in the event queue. */
void schedule_ble_events(BLE::OnEventsToProcessCallbackContext *context) {
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
