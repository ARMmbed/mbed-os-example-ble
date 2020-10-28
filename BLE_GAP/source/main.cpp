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

/** This example demonstrates all the basic setup required
 *  to advertise and scan.
 *
 *  It contains a single class that performs both scans and advertisements.
 *
 *  The demonstrations happens in sequence, after each "mode" ends
 *  the demo jumps to the next mode to continue.
 *
 *  You may connect to the device during advertising and if you advertise
 *  this demo will try to connect during the scanning phase. Connection
 *  will terminate the phase early. At the end of the phase some stats
 *  will be shown about the phase.
 */

/* demo config */
/* you can adjust these parameters and see the effect on the performance */

/* Advertising parameters are mainly defined by an advertising type and
 * and an interval between advertisements. Lower interval increases the
 * chances of being seen at the cost of increased power usage.
 *
 * The Bluetooth controller may run concurrent operations with the radio;
 * to help it, a minimum and maximum advertising interval should be
 * provided.
 *
 * Most bluetooth time units are specific to each operation. For example
 * adv_interval_t is expressed in multiples of 625 microseconds. If precision
 * is not require you may use a conversion from milliseconds.
 */
static const ble::AdvertisingParameters advertising_params(
    ble::advertising_type_t::CONNECTABLE_UNDIRECTED,
    ble::adv_interval_t(ble::millisecond_t(25)), /* this could also be expressed as ble::adv_interval_t(40) */
    ble::adv_interval_t(ble::millisecond_t(50)) /* this could also be expressed as ble::adv_interval_t(80) */
);

/* if the controller support it we can advertise multiple sets */
static const ble::AdvertisingParameters extended_advertising_params(
    ble::advertising_type_t::NON_CONNECTABLE_UNDIRECTED,
    ble::adv_interval_t(600),
    ble::adv_interval_t(800)
);

static const std::chrono::milliseconds advertising_duration = 10000ms;

/* Scanning happens repeatedly and is defined by:
 *  - The scan interval which is the time (in 0.625us) between each scan cycle.
 *  - The scan window which is the scanning time (in 0.625us) during a cycle.
 * If the scanning process is active, the local device sends scan requests
 * to discovered peer to get additional data.
 */
static const ble::ScanParameters scan_params(
    ble::phy_t::LE_1M,
    ble::scan_interval_t(80),
    ble::scan_window_t(60),
    false /* active scanning */
);

static const ble::scan_duration_t scan_duration(ble::millisecond_t(10000));

/* config end */

events::EventQueue event_queue;

using namespace std::literals::chrono_literals;

/* Delay between steps */
static const std::chrono::milliseconds delay = 3000ms;

/** Demonstrate advertising, scanning and connecting */
class GapDemo : private mbed::NonCopyable<GapDemo>, public ble::Gap::EventHandler
{
public:
    GapDemo(BLE& ble, events::EventQueue& event_queue) :
        _ble(ble),
        _gap(ble.gap()),
        _event_queue(event_queue)
    {
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
        /* handle gap events */
        _gap.setEventHandler(this);

        ble_error_t error = _ble.init(this, &GapDemo::on_init_complete);
        if (error) {
            print_error(error, "Error returned by BLE::init");
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
            print_error(event->error, "Error during the initialisation");
            return;
        }

        print_mac_address();

        /* setup the default phy used in connection to 2M to reduce power consumption */
        if (_gap.isFeatureSupported(ble::controller_supported_features_t::LE_2M_PHY)) {
            ble::phy_set_t phys(/* 1M */ false, /* 2M */ true, /* coded */ false);

            ble_error_t error = _gap.setPreferredPhys(/* tx */&phys, /* rx */&phys);

            /* PHY 2M communication will only take place if both peers support it */
            if (error) {
                print_error(error, "GAP::setPreferedPhys failed");
            }
        } else {
            /* otherwise it will use 1M by default */
        }

        /* all calls are serialised on the user thread through the event queue */
        _event_queue.call(this, &GapDemo::advertise);
    }

    /** Set up and start advertising */
    void advertise()
    {
        ble_error_t error = _gap.setAdvertisingParameters(ble::LEGACY_ADVERTISING_HANDLE, advertising_params);
        if (error) {
            print_error(error, "Gap::setAdvertisingParameters() failed");
            return;
        }

        /* to create a payload we'll use a helper class that builds a valid payload */
        /* AdvertisingDataSimpleBuilder is a wrapper over AdvertisingDataBuilder that allocated the buffer for us */
        ble::AdvertisingDataSimpleBuilder<ble::LEGACY_ADVERTISING_MAX_SIZE> data_builder;

        /* builder methods can be chained together as they return the builder object */
        data_builder.setFlags().setName("Legacy Set");

        /* Set payload for the set */
        error = _gap.setAdvertisingPayload(ble::LEGACY_ADVERTISING_HANDLE, data_builder.getAdvertisingData());
        if (error) {
            print_error(error, "Gap::setAdvertisingPayload() failed");
            return;
        }

        /* Start advertising the set */
        error = _gap.startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);
        if (error) {
            print_error(error, "Gap::startAdvertising() failed");
            return;
        }

        printf(
            "\r\nAdvertising started (type: 0x%x, interval: [%d : %d]ms)\r\n",
            advertising_params.getType(),
            advertising_params.getMinPrimaryInterval().valueInMs(),
            advertising_params.getMaxPrimaryInterval().valueInMs()
        );

        /* if we support extended advertising we'll also additionally advertise another set at the same time */
        if (_gap.isFeatureSupported(ble::controller_supported_features_t::LE_EXTENDED_ADVERTISING)) {
            /* With Bluetooth 5; it is possible to advertise concurrently multiple
             * payloads at different rate. The combination of payload and its associated
             * parameters is named an advertising set. To refer to these advertising
             * sets the Bluetooth system use an advertising set handle that needs to
             * be created first.
             * The only exception is the legacy advertising handle which is usable
             * on Bluetooth 4 and Bluetooth 5 system. It is created at startup and
             * its lifecycle is managed by the system.
             */
            ble_error_t error = _gap.createAdvertisingSet(&_extended_adv_handle, extended_advertising_params);
            if (error) {
                print_error(error, "Gap::createAdvertisingSet() failed");
                return;
            }

            /* we can reuse the builder, we just replace the name */
            data_builder.setName("Extended Set");

            /* Set payload for the set */
            error = _gap.setAdvertisingPayload(_extended_adv_handle, data_builder.getAdvertisingData());
            if (error) {
                print_error(error, "Gap::setAdvertisingPayload() failed");
                return;
            }

            /* Start advertising the set */
            error = _gap.startAdvertising(_extended_adv_handle);
            if (error) {
                print_error(error, "Gap::startAdvertising() failed");
                return;
            }

            printf(
                "Advertising started (type: 0x%x, interval: [%d : %d]ms)\r\n",
                extended_advertising_params.getType(),
                extended_advertising_params.getMinPrimaryInterval().valueInMs(),
                extended_advertising_params.getMaxPrimaryInterval().valueInMs()
            );
        }

        _demo_duration.reset();
        _demo_duration.start();

        /* this will stop advertising if no connection takes place in the meantime */
        _cancel_handle = _event_queue.call_in(advertising_duration, [this]{ end_advertising_mode(); });
    }

    /** Set up and start scanning */
    void scan()
    {
        ble_error_t error = _gap.setScanParameters(scan_params);
        if (error) {
            print_error(error, "Error caused by Gap::setScanParameters");
            return;
        }

        /* start scanning and attach a callback that will handle advertisements
         * and scan requests responses */
        error = _gap.startScan(scan_duration);
        if (error) {
            print_error(error, "Error caused by Gap::startScan");
            return;
        }

        printf("\r\nScanning started (interval: %dms, window: %dms, timeout: %dms).\r\n",
               scan_params.get1mPhyConfiguration().getInterval().valueInMs(),
               scan_params.get1mPhyConfiguration().getWindow().valueInMs(),
               scan_duration.valueInMs());

        _demo_duration.reset();
        _demo_duration.start();
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

        ble::AdvertisingDataParser adv_parser(event.getPayload());

        /* parse the advertising payload, looking for a discoverable device */
        while (adv_parser.hasNext()) {
            ble::AdvertisingDataParser::element_t field = adv_parser.next();

            /* skip non discoverable device */
            if (field.type != ble::adv_data_type_t::FLAGS ||
                field.value.size() != 1 ||
                !ble::adv_data_flags_t(field.value[0]).getGeneralDiscoverable()) {
                continue;
            }

            /* connect to a discoverable device */

            /* abort timeout as the mode will end on disconnection */
            _event_queue.cancel(_cancel_handle);

            printf("We found a connectable device\r\n");
            ble_error_t error = _gap.connect(
                event.getPeerAddressType(),
                event.getPeerAddress(),
                ble::ConnectionParameters() // use the default connection parameters
            );
            if (error) {
                print_error(error, "Error caused by Gap::connect");
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
        printf("Stopped scanning due to timeout parameter\r\n");
        _event_queue.call(this, &GapDemo::end_scanning_mode);
    }

    /** This is called by Gap to notify the application we connected,
     *  in our case it immediately disconnects */
    virtual void onConnectionComplete(const ble::ConnectionCompleteEvent &event)
    {
        _is_connecting = false;
        _demo_duration.stop();

        if (!_is_in_scanning_phase) {
            /* if we have more than one advertising sets one of them might still be active */
            if (_extended_adv_handle != ble::INVALID_ADVERTISING_HANDLE) {
                /* if it's still active, stop it */
                if (_gap.isAdvertisingActive(_extended_adv_handle)) {
                    _gap.stopAdvertising(_extended_adv_handle);
                } else if (_gap.isAdvertisingActive(ble::LEGACY_ADVERTISING_HANDLE)) {
                    _gap.stopAdvertising(ble::LEGACY_ADVERTISING_HANDLE);
                }
            }
        }

        if (event.getStatus() != BLE_ERROR_NONE) {
            print_error(event.getStatus(), "Connection failed");
            return;
        }

        printf("Connected in %dms\r\n", _demo_duration.read_ms());

        /* cancel the connect timeout since we connected */
        _event_queue.cancel(_cancel_handle);

        _cancel_handle = _event_queue.call_in(
            delay,
            [this, handle=event.getConnectionHandle()]{
                _gap.disconnect(handle, ble::local_disconnection_reason_t::USER_TERMINATION);
            }
        );

    }

    /** This is called by Gap to notify the application we disconnected,
     *  in our case it calls next_demo_mode() to progress the demo */
    virtual void onDisconnectionComplete(const ble::DisconnectionCompleteEvent &event)
    {
        printf("Disconnected\r\n");

        /* if it wasn't us disconnecting then we should cancel our attempt */
        if (event.getReason() == ble::disconnection_reason_t::REMOTE_USER_TERMINATED_CONNECTION) {
            _event_queue.cancel(_cancel_handle);
        }

        if (_is_in_scanning_phase) {
            _event_queue.call(this, &GapDemo::end_scanning_mode);
        } else {
            _event_queue.call(this, &GapDemo::end_advertising_mode);
        }
    }

    /**
     * Implementation of Gap::EventHandler::onReadPhy
     */
    virtual void onReadPhy(
        ble_error_t error,
        ble::connection_handle_t connectionHandle,
        ble::phy_t txPhy,
        ble::phy_t rxPhy
    )
    {
        if (error) {
            printf(
                "Phy read on connection %d failed with error code %s\r\n",
                connectionHandle, BLE::errorToString(error)
            );
        } else {
            printf(
                "Phy read on connection %d - Tx Phy: %s, Rx Phy: %s\r\n",
                connectionHandle, phy_to_string(txPhy), phy_to_string(rxPhy)
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
    )
    {
        if (error) {
            printf(
                "Phy update on connection: %d failed with error code %s\r\n",
                connectionHandle, BLE::errorToString(error)
            );
        } else {
            printf(
                "Phy update on connection %d - Tx Phy: %s, Rx Phy: %s\r\n",
                connectionHandle, phy_to_string(txPhy), phy_to_string(rxPhy)
            );
        }
    }

    /**
     * Implementation of Gap::EventHandler::onDataLengthChange
     */
    virtual void onDataLengthChange(
        ble::connection_handle_t connectionHandle,
        uint16_t txSize,
        uint16_t rxSize
    )
    {
        printf(
            "Data length changed on the connection %d.\r\n"
            "Maximum sizes for over the air packets are:\r\n"
            "%d octets for transmit and %d octets for receive.\r\n",
            connectionHandle, txSize, rxSize
        );
    }

private:
    /** Finish the mode by shutting down advertising or scanning and move to the next mode. */
    void end_scanning_mode()
    {
        print_scanning_performance();
        ble_error_t error = _gap.stopScan();

        if (error) {
            print_error(error, "Error caused by Gap::stopScan");
        }

        _is_in_scanning_phase = false;
        _scan_count = 0;

        _event_queue.call_in(delay, this, &GapDemo::advertise);
    }

    void end_advertising_mode()
    {
        print_advertising_performance();

        _gap.stopAdvertising(ble::LEGACY_ADVERTISING_HANDLE);

        if (_extended_adv_handle != ble::INVALID_ADVERTISING_HANDLE) {
            /* if it's still active, stop it */
            if (_gap.isAdvertisingActive(_extended_adv_handle)) {
                ble_error_t error = _gap.stopAdvertising(_extended_adv_handle);
                if (error) {
                    print_error(error, "Error caused by Gap::stopAdvertising");
                }
            }

            ble_error_t error = _gap.destroyAdvertisingSet(_extended_adv_handle);
            if (error) {
                print_error(error, "Error caused by Gap::destroyAdvertisingSet");
            }

            _extended_adv_handle = ble::INVALID_ADVERTISING_HANDLE;
        }

        _is_in_scanning_phase = true;

        _event_queue.call_in(delay, [this]{ scan(); });
    }

    /** print some information about our radio activity */
    void print_scanning_performance()
    {
        /* measure time from mode start, may have been stopped by timeout */
        uint16_t duration_ms = _demo_duration.read_ms();

        /* convert ms into timeslots for accurate calculation as internally
         * all durations are in timeslots (0.625ms) */
        uint16_t duration_ts = ble::scan_interval_t(ble::millisecond_t(duration_ms)).value();
        uint16_t interval_ts = scan_params.get1mPhyConfiguration().getInterval().value();
        uint16_t window_ts = scan_params.get1mPhyConfiguration().getWindow().value();
        /* this is how long we scanned for in timeslots */
        uint16_t rx_ts = (duration_ts / interval_ts) * window_ts;
        /* convert to milliseconds */
        uint16_t rx_ms = ble::scan_interval_t(rx_ts).valueInMs();

        printf(
            "We have scanned for %dms with an interval of %d"
            " timeslots and a window of %d timeslots\r\n",
            duration_ms, interval_ts, window_ts
        );

        printf("We have been listening on the radio for at least %dms\r\n", rx_ms);
    }

    /** print some information about our radio activity */
    void print_advertising_performance()
    {
        /* measure time from mode start, may have been stopped by timeout */
        uint16_t duration_ms = _demo_duration.read_ms();

        /* convert ms into timeslots for accurate calculation as internally
         * all durations are in timeslots (0.625ms) */
        uint16_t duration_ts = ble::adv_interval_t(ble::millisecond_t(duration_ms)).value();
        uint16_t interval_ts = advertising_params.getMaxPrimaryInterval().value();
        /* this is how many times we advertised */
        uint16_t events = (duration_ts / interval_ts);
        uint16_t extended_events = 0;

        if (_extended_adv_handle != ble::INVALID_ADVERTISING_HANDLE) {
            duration_ts = ble::adv_interval_t(ble::millisecond_t(duration_ms)).value();
            interval_ts = extended_advertising_params.getMaxPrimaryInterval().value();
            /* this is how many times we advertised */
            extended_events = (duration_ts / interval_ts);
        }

        printf("We have advertised for %dms\r\n", duration_ms);

        /* non-scannable and non-connectable advertising
         * skips rx events saving on power consumption */
        if (advertising_params.getType() == ble::advertising_type_t::NON_CONNECTABLE_UNDIRECTED) {
            printf("We created at least %d tx events\r\n", events);
        } else {
            printf("We created at least %d tx and rx events\r\n", events);
        }
        if (extended_events) {
            if (extended_advertising_params.getType() == ble::advertising_type_t::NON_CONNECTABLE_UNDIRECTED) {
                printf("We created at least %d tx events with extended advertising\r\n", extended_events);
            } else {
                printf("We created at least %d tx and rx events with extended advertising\r\n", extended_events);
            }
        }
    }

private:
    BLE &_ble;
    ble::Gap &_gap;
    events::EventQueue &_event_queue;

    /* Keep track of our progress through demo modes */
    bool _is_in_scanning_phase = false;
    bool _is_connecting = false;

    /* Remember the call id of the function on _event_queue
     * so we can cancel it if we need to end the phase early */
    int _cancel_handle = 0;

    /* Measure performance of our advertising/scanning */
    Timer _demo_duration;
    size_t _scan_count = 0;

    ble::advertising_handle_t _extended_adv_handle = ble::INVALID_ADVERTISING_HANDLE;
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

    GapDemo demo(ble, event_queue);

    demo.run();

    return 0;
}
