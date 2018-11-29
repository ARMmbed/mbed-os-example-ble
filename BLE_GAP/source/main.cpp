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

/** This example demonstrates all the basic setup required
 *  to advertise, scan and connect to other devices.
 *
 *  It contains a single class that performs both scans and advertisements.
 *
 *  The demonstrations happens in sequence, after each "mode" ends
 *  the demo jumps to the next mode to continue. There are several modes
 *  that show scanning and several showing advertising. These are configured
 *  according to the two arrays containing parameters. During scanning
 *  a connection will be made to a connectable device upon its discovery.
 */

events::EventQueue event_queue;

/* Duration of each mode in milliseconds */
static const size_t MODE_DURATION_MS      = 6000;

/* Time between each mode in milliseconds */
static const size_t TIME_BETWEEN_MODES_MS = 2000;

/* how long to wait before disconnecting in milliseconds */
static const size_t CONNECTION_DURATION   = 3000;

/* how many advertising sets we want to crate at once */
static const uint8_t ADV_SET_NUMBER       = 2;

static const uint16_t MAX_ADVERTISING_PAYLOAD_SIZE = 1000;

typedef struct {
    ble::advertising_type_t type;
    ble::adv_interval_t min_interval;
    ble::adv_interval_t max_interval;
} DemoAdvParams_t;

typedef struct {
    ble::scan_interval_t interval;
    ble::scan_window_t   window;
    ble::scan_duration_t duration;
    bool active;
} DemoScanParam_t;

/** the entries in this array are used to configure our advertising
 *  parameters for each of the modes we use in our demo */
static const DemoAdvParams_t advertising_params[] = {
/*    advertising type                                   | min interval - 0.625us  | max interval - 0.625us   */
    { ble::advertising_type_t::CONNECTABLE_UNDIRECTED,      ble::adv_interval_t(40), ble::adv_interval_t(80)  },
    { ble::advertising_type_t::SCANNABLE_UNDIRECTED,       ble::adv_interval_t(100), ble::adv_interval_t(200) },
    { ble::advertising_type_t::NON_CONNECTABLE_UNDIRECTED, ble::adv_interval_t(100), ble::adv_interval_t(200) }
};

/* when we cycle through all our advertising modes we will move to scanning modes */

/** the entries in this array are used to configure our scanning
 *  parameters for each of the modes we use in our demo */
static const DemoScanParam_t scanning_params[] = {
/*                      interval                  window                   duration  active */
/*                      0.625ms                  0.625ms                       10ms         */
    {   ble::scan_interval_t(4),   ble::scan_window_t(4),   ble::scan_duration_t(0), false },
    { ble::scan_interval_t(160), ble::scan_window_t(100), ble::scan_duration_t(300), false },
    { ble::scan_interval_t(160),  ble::scan_window_t(40),   ble::scan_duration_t(0), true  },
    { ble::scan_interval_t(500),  ble::scan_window_t(10),   ble::scan_duration_t(0), false }
};

/* helper that gets the number of items in arrays */
template<class T, size_t N>
size_t size(const T (&)[N])
{
    return N;
}

/** Demonstrate advertising, scanning and connecting
 */
class GapDemo : private mbed::NonCopyable<GapDemo>, public ble::Gap::EventHandler
{
public:
    GapDemo(BLE& ble, events::EventQueue& event_queue) :
        _ble(ble),
        _event_queue(event_queue),
        _led1(LED1, 0),
        _set_index(0),
        _is_in_scanning_mode(true),
        _is_connecting(false),
        _on_duration_end_id(0),
        _scan_count(0),
        _blink_event(0) {
        for (uint8_t i = 0; i < size(_adv_handles); ++i) {
            _adv_handles[i] = ble::INVALID_ADVERTISING_HANDLE;
        }
    }

    ~GapDemo()
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
        _ble.gap().setEventHandler(this);

        ble_error_t error = _ble.init(this, &GapDemo::on_init_complete);
        if (error) {
            print_error(error, "Error returned by BLE::init");
            return;
        }

        /* to show we're running we'll blink every 500ms */
        _blink_event = _event_queue.call_every(500, this, &GapDemo::blink);

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

        print_mac_address();

        /* setup the default phy used in connection to 2M to reduce power consumption */
        if (is_2m_phy_supported()) {
            ble::phy_set_t phys(/* 1M */ false, /* 2M */ true, /* coded */ false);

            ble_error_t error = _ble.gap().setPreferredPhys(/* tx */&phys, /* rx */&phys);
            if (error) {
                print_error(error, "GAP::setPreferedPhys failed");
            }
        }

        /* all calls are serialised on the user thread through the event queue */
        _event_queue.call(this, &GapDemo::demo_mode_start);
    }

    /** queue up start of the current demo mode */
    void demo_mode_start()
    {
        if (_is_in_scanning_mode) {
            _event_queue.call(this, &GapDemo::scan);
        } else {
            _event_queue.call(this, &GapDemo::advertise);
        }

        /* for performance measurement keep track of duration of the demo mode */
        _demo_duration.start();
        /* keep track of our state */
        _is_connecting = false;

        /* queue up next demo mode */
        _on_duration_end_id = _event_queue.call_in(
            MODE_DURATION_MS,
            this,
            &GapDemo::end_demo_mode
        );

        printf("\r\n");
    }

    /** Set up and start advertising */
    void advertise()
    {
        const DemoAdvParams_t &adv_params = advertising_params[_set_index];

        /*
         * Advertising parameters are mainly defined by an advertising type and
         * and an interval between advertisements. lower interval increases the
         * chances of being seen at the cost of more power.
         * The Bluetooth controller may run concurrent operations with the radio;
         * to help it, a minimum and maximum advertising interval should be
         * provided.
         *
         * With Bluetooth 5; it is possible to advertise concurrently multiple
         * payloads at different rate. The combination of payload and its associated
         * parameters is named an advertising set. To refer to these advertising
         * sets the Bluetooth system use an advertising set handle that needs to
         * be created first.
         * The only exception is the legacy advertising handle which is usable
         * on Bluetooth 4 and Bluetooth 5 system. It is created at startup and
         * its lifecycle is managed by the system.
         */
        ble_error_t error = _ble.gap().setAdvertisingParameters(
            ble::LEGACY_ADVERTISING_HANDLE,
            ble::AdvertisingParameters(
                adv_params.type,
                adv_params.min_interval,
                adv_params.max_interval
            )
        );
        if (error) {
            print_error(error, "Gap::setAdvertisingParameters() failed");
            return;
        }

        /* Set payload for the set */
        error = _ble.gap().setAdvertisingPayload(
            ble::LEGACY_ADVERTISING_HANDLE,
            ble::AdvertisingDataSimpleBuilder<ble::LEGACY_ADVERTISING_MAX_SIZE>()
                .setFlags()
                .setName("Legacy advertiser")
                .getAdvertisingData()
        );
        if (error) {
            print_error(error, "Gap::setAdvertisingPayload() failed");
            return;
        }

        /* Start advertising the set */
        error = _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);
        if (error) {
            print_error(error, "Gap::startAdvertising() failed");
            return;
        }

        printf("Advertising started (type: 0x%x, interval: [%d : %d]ms)\r\n",
            adv_params.type.value(),
            adv_params.min_interval.valueInMs(), adv_params.max_interval.valueInMs() );

        if (is_extended_advertising_supported()) {
            advertise_extended();
        }
    }

    void advertise_extended()
    {
        const DemoAdvParams_t &adv_params = advertising_params[_set_index];

        /* this is the memory backing for the payload */
        uint8_t adv_buffer[MAX_ADVERTISING_PAYLOAD_SIZE];

        /* how many sets */
        uint8_t max_adv_set = std::min(
            _ble.gap().getMaxAdvertisingSetNumber(),
            (uint8_t) size(_adv_handles)
        );

        /* how much payload in a set */
        uint16_t max_adv_size = std::min(
            (uint16_t) _ble.gap().getMaxAdvertisingDataLength(),
            MAX_ADVERTISING_PAYLOAD_SIZE
        );

        /* create and start all requested (and possible) advertising sets */
        /* Note: one advertising is reserved for legacy advertising (max_adv_set - 1) */
        for (uint8_t i = 0; i < (max_adv_set - 1); ++i) {
            /* create the advertising set with its parameter */
            /* this time we do not use legacy PDUs */
            ble_error_t error = _ble.gap().createAdvertisingSet(
                &_adv_handles[i],
                ble::AdvertisingParameters(
                    adv_params.type,
                    adv_params.min_interval,
                    adv_params.max_interval
                ).setUseLegacyPDU(false)
            );
            if (error) {
                print_error(error, "Gap::createAdvertisingSet() failed");
                return;
            }

            /* use the helper to build the payload */
            ble::AdvertisingDataBuilder adv_data_builder(
                adv_buffer,
                max_adv_size
            );

            /* set the flags */
            error = adv_data_builder.setFlags();
            if (error) {
                print_error(error, "AdvertisingDataBuilder::setFlags() failed");
                return;
            }

            /* set different name for each set */
            MBED_ASSERT(i < 9);
            char device_name[] = "Advertiser x";
            sprintf(device_name, "Advertiser %d", i%10);
            error = adv_data_builder.setName(device_name);
            if (error) {
                print_error(error, "AdvertisingDataBuilder::setName() failed");
                return;
            }

            /* Set payload for the set */
            error = _ble.gap().setAdvertisingPayload(
                _adv_handles[i],
                adv_data_builder.getAdvertisingData()
            );
            if (error) {
                print_error(error, "Gap::setAdvertisingPayload() failed");
                return;
            }

            /* Start advertising the set */
            error = _ble.gap().startAdvertising(
                _adv_handles[i]
            );
            if (error) {
                print_error(error, "Gap::startAdvertising() failed");
                return;
            }

            printf("Advertising started (type: 0x%x, interval: [%d : %d]ms)\r\n",
                adv_params.type.value(),
                adv_params.min_interval.valueInMs(), adv_params.max_interval.valueInMs() );
        }
    }

    /** Set up and start scanning */
    void scan()
    {
        const DemoScanParam_t &scan_params = scanning_params[_set_index];

        /*
         * Scanning happens repeatedly and is defined by:
         *  - The scan interval which is the time (in 0.625us) between each scan cycle.
         *  - The scan window which is the scanning time (in 0.625us) during a cycle.
         * If the scanning process is active, the local device sends scan requests
         * to discovered peer to get additional data.
         */
        ble_error_t error = _ble.gap().setScanParameters(
            ble::ScanParameters(
                ble::phy_t::LE_1M,   // scan on the 1M PHY
                scan_params.interval,
                scan_params.window,
                scan_params.active
            )
        );
        if (error) {
            print_error(error, "Error caused by Gap::setScanParameters");
            return;
        }

        /* start scanning and attach a callback that will handle advertisements
         * and scan requests responses */
        error = _ble.gap().startScan(
            ble::duplicates_filter_t::DISABLE,
            scan_params.duration
        );
        if (error) {
            print_error(error, "Error caused by Gap::startScan");
            return;
        }

        printf("Scanning started (interval: %dms, window: %dms, timeout: %dms).\r\n",
               scan_params.interval.valueInMs(), scan_params.window.valueInMs(), scan_params.duration.valueInMs());
    }

    /** Finish the mode by shutting down advertising or scanning and move to the next mode. */
    void end_demo_mode()
    {
        if (_is_in_scanning_mode) {
            end_scanning_mode();
        } else {
            end_advertising_mode();
        }

        /* alloted time has elapsed or be connected, move to next demo mode */
        _event_queue.call(this, &GapDemo::next_demo_mode);
    }

    /** Execute the disconnection */
    void do_disconnect(ble::connection_handle_t handle)
    {
        printf("Disconnecting\r\n");
        _ble.gap().disconnect(handle, ble::local_disconnection_reason_t::USER_TERMINATION);
    }

    bool is_2m_phy_supported()
    {
        return _ble.gap().isFeatureSupported(ble::controller_supported_features_t::LE_2M_PHY);
    }

    bool is_extended_advertising_supported()
    {
        return _ble.gap().isFeatureSupported(ble::controller_supported_features_t::LE_EXTENDED_ADVERTISING);
    }


private:
    /* Gap::EventHandler */

    /** Look at scan payload to find a peer device and connect to it */
    virtual void onAdvertisingReport(const ble::AdvertisingReportEvent &event)
    {
        /* keep track of scan events for performance reporting */
        _scan_count++;

        /* don't bother with analysing scan result if we're already connecting */
        if (_is_connecting) {
            return;
        }

        /* only look at events from devices at a close range */
        if (event.getRssi() < -65) {
            return;
        }

        ble::AdvertisingDataParser adv_parser(event.getAdvertisingData());

        /* parse the advertising payload, looking for a discoverable device */
        while (adv_parser.hasNext()) {
            ble::AdvertisingDataParser::element_t field = adv_parser.next();

            /* skip non discoverable device */
            if (field.type != ble::adv_data_type_t::FLAGS ||
                field.value.size() != 1 ||
                !(field.value[0] & GapAdvertisingData::LE_GENERAL_DISCOVERABLE)) {
                continue;
            }

            /* connect to a discoverable device */

            /* abort timeout as the mode will end on disconnection */
            _event_queue.cancel(_on_duration_end_id);

            printf("We found a connectable device\r\n");
            ble_error_t error = _ble.gap().connect(
                event.getPeerAddressType(),
                event.getPeerAddress(),
                ble::ConnectionParameters() // use the default connection parameters
            );
            if (error) {
                print_error(error, "Error caused by Gap::connect");
                /* since no connection will be attempted end the mode */
                _event_queue.call(this, &GapDemo::end_demo_mode);
                return;
            }

            /* we may have already scan events waiting
             * to be processed so we need to remember
             * that we are already connecting and ignore them */
            _is_connecting = true;

            return;
        }
    }

    virtual void onAdvertisingEnd(const ble::AdvertisingEndEvent &event)
    {
        if (event.isConnected()) {
            printf("Stopped advertising early due to connection\r\n");
        }
    }

    virtual void onScanTimeout(const ble::ScanTimeoutEvent&)
    {
        printf("Stopped scanning early due to timeout parameter\r\n");
        _demo_duration.stop();
    }

    /** This is called by Gap to notify the application we connected,
     *  in our case it immediately disconnects */
    virtual void onConnectionComplete(const ble::ConnectionCompleteEvent &event)
    {
        _demo_duration.stop();

        if (event.getStatus() == BLE_ERROR_NONE) {
            printf("Connected in %dms\r\n", _demo_duration.read_ms());

            /* cancel the connect timeout since we connected */
            _event_queue.cancel(_on_duration_end_id);

            _event_queue.call_in(
                CONNECTION_DURATION,
                this,
                &GapDemo::do_disconnect,
                event.getConnectionHandle()
            );
        } else {
            printf("Failed to connect after scanning %d advertisements\r\n", _scan_count);
            _event_queue.call(this, &GapDemo::end_demo_mode);
        }
    }

    /** This is called by Gap to notify the application we disconnected,
     *  in our case it calls next_demo_mode() to progress the demo */
    virtual void onDisconnectionComplete(const ble::DisconnectionEvent &event) {
        printf("Disconnected\r\n");

        /* we have successfully disconnected ending the demo, move to next mode */
        _event_queue.call(this, &GapDemo::end_demo_mode);
    }

    /**
     * Implementation of Gap::EventHandler::onReadPhy
     */
    virtual void onReadPhy(
        ble_error_t error,
        ble::connection_handle_t connectionHandle,
        ble::phy_t txPhy,
        ble::phy_t rxPhy
    ) {
        if (error) {
            printf(
                "Phy read on connection %d failed with error code %s\r\n",
                connectionHandle,
                BLE::errorToString(error)
            );
        } else {
            printf(
                "Phy read on connection %d - Tx Phy: %s, Rx Phy: %s\r\n",
                connectionHandle,
                phy_to_string(txPhy),
                phy_to_string(rxPhy)
            );
        }
    }

    /**
     * Implementation of Gap::EventHandler::onPhyUpdateComplete
     */
    virtual void onPhyUpdateComplete(
        ble_error_t error,
        ble::connection_handle_t connectionHandle,
        ble::phy_t txPhy,
        ble::phy_t rxPhy
    ) {
        if (error) {
            printf(
                "Phy update on connection: %d failed with error code %s\r\n",
                connectionHandle,
                BLE::errorToString(error)
            );
        } else {
            printf(
                "Phy update on connection %d - Tx Phy: %s, Rx Phy: %s\r\n",
                connectionHandle,
                phy_to_string(txPhy),
                phy_to_string(rxPhy)
            );
        }
    }

private:

    /** Clean up internal state after last run, cycle to the next mode and launch it */
    void next_demo_mode()
    {
        /* reset the demo ready for the next mode */
        _scan_count = 0;
        _demo_duration.stop();
        _demo_duration.reset();

        /* cycle through all demo modes */
        _set_index++;

        /* switch between advertising and scanning when we go
         * through all the params in the array */
        if (_set_index >= (_is_in_scanning_mode ? size(scanning_params) : size(advertising_params))) {
            _set_index = 0;
            _is_in_scanning_mode = !_is_in_scanning_mode;
        }

        _ble.shutdown();
        _event_queue.cancel(_blink_event);
        _event_queue.break_dispatch();
    }

    /** Finish the mode by shutting down advertising or scanning and move to the next mode. */
    void end_scanning_mode()
    {
        print_scanning_performance();
        ble_error_t error = _ble.gap().stopScan();

        if (error) {
            print_error(error, "Error caused by Gap::stopScan");
        }
    }

    void end_advertising_mode()
    {
        print_advertising_performance();
        /* go through all the created advertising sets to shut them down and remove them */
        for (uint8_t i = 0; i < size(_adv_handles); ++i) {
            /* check if the set has been sucesfully created */
            if (_adv_handles[i] != ble::INVALID_ADVERTISING_HANDLE) {
                /* if it's still active, stop it */
                if (_ble.gap().isAdvertisingActive(_adv_handles[i])) {
                    ble_error_t error = _ble.gap().stopAdvertising(_adv_handles[i]);

                    if (error) {
                        print_error(error, "Error caused by Gap::stopAdvertising");
                        return;
                    }
                }
                /* you cannot destroy or create the legacy advertising set,
                 * it's always available, others may be removed when no longer needed */
                if (_adv_handles[i] != ble::LEGACY_ADVERTISING_HANDLE) {
                    ble_error_t error = _ble.gap().destroyAdvertisingSet(_adv_handles[i]);

                    if (error) {
                        print_error(error, "Error caused by Gap::destroyAdvertisingSet");
                        return;
                    }
                }
            }
        }
    }

    /** print some information about our radio activity */
    void print_scanning_performance()
    {
        /* measure time from mode start, may have been stopped by timeout */
        uint16_t duration_ms = _demo_duration.read_ms();

        /* convert ms into timeslots for accurate calculation as internally
         * all durations are in timeslots (0.625ms) */
        uint16_t duration_ts = ble::scan_interval_t(ble::millisecond_t(duration_ms)).value();
        uint16_t interval_ts = scanning_params[_set_index].interval.value();
        uint16_t window_ts = scanning_params[_set_index].window.value();
        /* this is how long we scanned for in timeslots */
        uint16_t rx_ts = (duration_ts / interval_ts) * window_ts;
        /* convert to milliseconds */
        uint16_t rx_ms = ble::scan_interval_t(rx_ts).valueInMs();

        printf("We have scanned for %dms with an interval of %d"
               " timeslots and a window of %d timeslots\r\n",
            duration_ms, interval_ts, window_ts);

        printf("We have been listening on the radio for at least %dms\r\n", rx_ms);
    }

    /** print some information about our radio activity */
    void print_advertising_performance()
    {
        /* measure time from mode start, may have been stopped by timeout */
        uint16_t duration_ms = _demo_duration.read_ms();
        uint8_t number_of_active_sets = 0;

        for (uint8_t i = 0; i < size(_adv_handles); ++i) {
            if (_adv_handles[i] != ble::INVALID_ADVERTISING_HANDLE) {
                if (_ble.gap().isAdvertisingActive(_adv_handles[i])) {
                    number_of_active_sets++;
                }
            }
        }

        /* convert ms into timeslots for accurate calculation as internally
         * all durations are in timeslots (0.625ms) */
        uint16_t duration_ts = ble::adv_interval_t(ble::millisecond_t(duration_ms)).value();
        uint16_t interval_ts = advertising_params[_set_index].min_interval.value();
        /* this is how many times we advertised */
        uint16_t events = (duration_ts / interval_ts) * number_of_active_sets;

        printf("We have advertised for %dms with an interval of %d timeslots\r\n",
            duration_ms, interval_ts);

        if (number_of_active_sets > 1) {
            printf("We had %d active advertising sets\r\n", number_of_active_sets);
        }

        /* non-scannable and non-connectable advertising
         * skips rx events saving on power consumption */
        if (advertising_params[_set_index].type == ble::advertising_type_t::NON_CONNECTABLE_UNDIRECTED) {
            printf("We created at least %d tx events\r\n", events);
        } else {
            printf("We created at least %d tx and rx events\r\n", events);
        }
    }

    /** Blink LED to show we're running */
    void blink(void)
    {
        _led1 = !_led1;
    }

private:
    BLE                &_ble;
    events::EventQueue &_event_queue;
    DigitalOut          _led1;

    /* Keep track of our progress through demo modes */
    size_t              _set_index;
    bool                _is_in_scanning_mode;
    bool                _is_connecting;

    /* Remember the call id of the function on _event_queue
     * so we can cancel it if we need to end the mode early */
    int                 _on_duration_end_id;

    /* Measure performance of our advertising/scanning */
    Timer               _demo_duration;
    size_t              _scan_count;

    int                 _blink_event;

    ble::advertising_handle_t _adv_handles[ADV_SET_NUMBER];
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

    GapDemo demo(ble, event_queue);

    while (1) {
        demo.run();
        wait_ms(TIME_BETWEEN_MODES_MS);
        printf("\r\nStarting next GAP demo mode\r\n");
    };

    return 0;
}
