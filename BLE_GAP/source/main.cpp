/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
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

static const uint8_t DEVICE_NAME[]        = "GAP_device";

/* Duration of each mode in milliseconds */
static const size_t MODE_DURATION_MS      = 6000;

/* Time between each mode in milliseconds */
static const size_t TIME_BETWEEN_MODES_MS = 2000;

/* how long to wait before disconnecting in milliseconds */
static const size_t CONNECTION_DURATION = 3000;

typedef struct {
    GapAdvertisingParams::AdvertisingType_t adv_type;
    uint16_t interval;
    uint16_t timeout;
} AdvModeParam_t;

typedef struct {
    uint16_t interval;
    uint16_t window;
    uint16_t timeout;
    bool active;
} ScanModeParam_t;

/** the entries in this array are used to configure our advertising
 *  parameters for each of the modes we use in our demo */
static const AdvModeParam_t advertising_params[] = {
    /*            advertising type                        interval  timeout */
    { GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED,      40,/*ms*/ 3/*s*/},
    { GapAdvertisingParams::ADV_SCANNABLE_UNDIRECTED,       100,       4     },
    { GapAdvertisingParams::ADV_NON_CONNECTABLE_UNDIRECTED, 100,       0     }
};

/* when we cycle through all our advertising modes we will move to scanning modes */

/** the entries in this array are used to configure our scanning
 *  parameters for each of the modes we use in our demo */
static const ScanModeParam_t scanning_params[] = {
/* interval      window    timeout       active */
    {   4,/*ms*/   4,/*ms*/   0,/*s*/    false },
    { 160,       100,         3,         false },
    { 160,        40,         0,         true  },
    { 500,        10,         0,         false }
};

/* parameters to use when attempting to connect to maximise speed of connection */
static const GapScanningParams connection_scan_params(
    GapScanningParams::SCAN_INTERVAL_MAX,
    GapScanningParams::SCAN_WINDOW_MAX,
    3,
    false
);

/* get number of items in our arrays */
static const size_t SCAN_PARAM_SET_MAX =
    sizeof(scanning_params) / sizeof(GapScanningParams);
static const size_t ADV_PARAM_SET_MAX  =
    sizeof(advertising_params) / sizeof(GapAdvertisingParams);

static const char* to_string(Gap::Phy_t phy) {
    switch(phy.value()) {
        case Gap::Phy_t::LE_1M:
            return "LE 1M";
        case Gap::Phy_t::LE_2M:
            return "LE 2M";
        case Gap::Phy_t::LE_CODED:
            return "LE coded";
        default:
            return "invalid PHY";
    }
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
        _is_in_scanning_mode(false),
        _is_connecting(false),
        _on_duration_end_id(0),
        _scan_count(0) { }

    ~GapDemo()
    {
        if (_ble.hasInitialized()) {
            _ble.shutdown();
        }
    }

    /** Start BLE interface initialisation */
    void run()
    {
        ble_error_t error;

        if (_ble.hasInitialized()) {
            printf("Ble instance already initialised.\r\n");
            return;
        }

        /* handle timeouts, for example when connection attempts fail */
        _ble.gap().onTimeout(
            makeFunctionPointer(this, &GapDemo::on_timeout)
        );

        /* handle gap events */
        _ble.gap().setEventHandler(this);

        error = _ble.init(this, &GapDemo::on_init_complete);

        if (error) {
            printf("Error returned by BLE::init.\r\n");
            return;
        }

        /* to show we're running we'll blink every 500ms */
        _event_queue.call_every(500, this, &GapDemo::blink);

        /* this will not return until shutdown */
        _event_queue.dispatch_forever();
    }

private:
    /** This is called when BLE interface is initialised and starts the first mode */
    void on_init_complete(BLE::InitializationCompleteCallbackContext *event)
    {
        if (event->error) {
            printf("Error during the initialisation\r\n");
            return;
        }

        /* print device address */
        Gap::AddressType_t addr_type;
        Gap::Address_t addr;
        _ble.gap().getAddress(&addr_type, addr);
        printf("Device address: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
               addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);

        /* setup the default phy used in connection to 2M to reduce power consumption */
        Gap::PhySet_t tx_phys(/* 1M */ false, /* 2M */ true, /* coded */ false);
        Gap::PhySet_t rx_phys(/* 1M */ false, /* 2M */ true, /* coded */ false);
        ble_error_t err = _ble.gap().setPreferredPhys(&tx_phys, &rx_phys);
        if (err) {
            printf("INFO: GAP::setPreferedPhys failed with error code %s", BLE::errorToString(err));
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
            MODE_DURATION_MS, this, &GapDemo::on_duration_end
        );

        printf("\r\n");
    }

    /** Set up and start advertising */
    void advertise()
    {
        ble_error_t error;
        GapAdvertisingData advertising_data;

        /* add advertising flags */
        advertising_data.addFlags(GapAdvertisingData::LE_GENERAL_DISCOVERABLE
                                  | GapAdvertisingData::BREDR_NOT_SUPPORTED);

        /* add device name */
        advertising_data.addData(
            GapAdvertisingData::COMPLETE_LOCAL_NAME,
            DEVICE_NAME,
            sizeof(DEVICE_NAME)
        );

        error = _ble.gap().setAdvertisingPayload(advertising_data);

        if (error) {
            printf("Error during Gap::setAdvertisingPayload\r\n");
            return;
        }

        /* set the advertising parameters according to currently selected set,
         * see @AdvertisingType_t for explanation of modes */
        GapAdvertisingParams::AdvertisingType_t adv_type =
            advertising_params[_set_index].adv_type;

        /* how many milliseconds between advertisements, lower interval
         * increases the chances of being seen at the cost of more power */
        uint16_t interval = advertising_params[_set_index].interval;

        /* advertising will continue for this many seconds or until connected */
        uint16_t timeout = advertising_params[_set_index].timeout;

        _ble.gap().setAdvertisingType(adv_type);
        _ble.gap().setAdvertisingInterval(interval);
        _ble.gap().setAdvertisingTimeout(timeout);

        error = _ble.gap().startAdvertising();

        if (error) {
            printf("Error during Gap::startAdvertising.\r\n");
            return;
        }

        printf("Advertising started (type: 0x%x, interval: %dms, timeout: %ds)\r\n",
               adv_type, interval, timeout);
    }

    /** Set up and start scanning */
    void scan()
    {
        ble_error_t error;

        /* scanning happens repeatedly, interval is the number of milliseconds
         * between each cycle of scanning */
        uint16_t interval = scanning_params[_set_index].interval;

        /* number of milliseconds we scan for each time we enter
         * the scanning cycle after the interval set above */
        uint16_t window = scanning_params[_set_index].window;

        /* how long to repeat the cycles of scanning in seconds */
        uint16_t timeout = scanning_params[_set_index].timeout;

        /* active scanning will send a scan request to any scanable devices that
         * we see advertising */
        bool active = scanning_params[_set_index].active;

        /* set the scanning parameters according to currently selected set */
        error = _ble.gap().setScanParams(interval, window, timeout, active);

        if (error) {
            printf("Error during Gap::setScanParams\r\n");
            return;
        }

        /* start scanning and attach a callback that will handle advertisements
         * and scan requests responses */
        error = _ble.gap().startScan();

        if (error) {
            printf("Error during Gap::startScan\r\n");
            return;
        }

        printf("Scanning started (interval: %dms, window: %dms, timeout: %ds).\r\n",
               interval, window, timeout);
    }

    /** After a set duration this cycles to the next demo mode
     *  unless a connection happened first */
    void on_duration_end()
    {
        print_performance();

        /* alloted time has elapsed, move to next demo mode */
        _event_queue.call(this, &GapDemo::demo_mode_end);
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

        /* parse the advertising payload, looking for a discoverable device */
        for (uint8_t i = 0; i < event.getAdvertisingData().size(); ++i) {
            /* The advertising payload is a collection of key/value records where
             * byte 0: length of the record excluding this byte
             * byte 1: The key, it is the type of the data
             * byte [2..N] The value. N is equal to byte0 - 1 */
            const uint8_t record_length = event.getAdvertisingData()[i];
            if (record_length == 0) {
                continue;
            }
            const uint8_t type = event.getAdvertisingData()[i + 1];
            const uint8_t *value = event.getAdvertisingData().data() + i + 2;

            /* connect to a discoverable device */
            if ((type == GapAdvertisingData::FLAGS)
                && (*value & GapAdvertisingData::LE_GENERAL_DISCOVERABLE)) {

                /* abort timeout as the mode will end on disconnection */
                _event_queue.cancel(_on_duration_end_id);

                printf("We found a connectable device\r\n");

                const ble::ConnectionParameters connection_params;

                ble_error_t error = _ble.gap().connect(
                    event.getPeerAddressType().getTargetAddressType(),
                    event.getPeerAddress(),
                    connection_params
                );

                if (error) {
                    printf("Error during Gap::connect\r\n");
                    /* since no connection will be attempted end the mode */
                    _event_queue.call(this, &GapDemo::demo_mode_end);
                    return;
                }

                /* we may have already scan events waiting
                 * to be processed so we need to remember
                 * that we are already connecting and ignore them */
                _is_connecting = true;

                return;
            }

            i += record_length;
        }
    }

    /** This is called by Gap to notify the application we connected,
     *  in our case it immediately disconnects */
    virtual void onConnectionComplete(const ble::ConnectionCompleteEvent &event)
    {
        print_performance();

        printf("Connected in %dms\r\n", _demo_duration.read_ms());

        /* cancel the connect timeout since we connected */
        _event_queue.cancel(_on_duration_end_id);

        _event_queue.call_in(
            CONNECTION_DURATION, &_ble.gap(), &Gap::disconnect, Gap::REMOTE_USER_TERMINATED_CONNECTION
        );
    }

    /** This is called by Gap to notify the application we disconnected,
     *  in our case it calls demo_mode_end() to progress the demo */
    virtual void onDisconnection(const ble::DisconnectionEvent &event) {
        printf("Disconnected\r\n");

        /* we have successfully disconnected ending the demo, move to next mode */
        _event_queue.call(this, &GapDemo::demo_mode_end);
    }

    /**
     * Implementation of Gap::EventHandler::onReadPhy
     */
    virtual void onReadPhy(
        ble_error_t error,
        Gap::Handle_t connectionHandle,
        Gap::Phy_t txPhy,
        Gap::Phy_t rxPhy
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
                to_string(txPhy),
                to_string(rxPhy)
            );
        }
    }

    /**
     * Implementation of Gap::EventHandler::onPhyUpdateComplete
     */
    virtual void onPhyUpdateComplete(
        ble_error_t error,
        Gap::Handle_t connectionHandle,
        Gap::Phy_t txPhy,
        Gap::Phy_t rxPhy
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
                to_string(txPhy),
                to_string(rxPhy)
            );
        }
    }

private:
    /** called if timeout is reached during advertising, scanning
     *  or connection initiation */
    void on_timeout(const Gap::TimeoutSource_t source)
    {
        _demo_duration.stop();

        switch (source) {
            case Gap::TIMEOUT_SRC_ADVERTISING:
                printf("Stopped advertising early due to timeout parameter\r\n");
                break;
            case Gap::TIMEOUT_SRC_SCAN:
                printf("Stopped scanning early due to timeout parameter\r\n");
                break;
            case Gap::TIMEOUT_SRC_CONN:
                printf("Failed to connect after scanning %d advertisements\r\n", _scan_count);
                _event_queue.call(this, &GapDemo::print_performance);
                _event_queue.call(this, &GapDemo::demo_mode_end);
                break;
            default:
                printf("Unexpected timeout\r\n");
                break;
        }
    }

    /** clean up after last run, cycle to the next mode and launch it */
    void demo_mode_end()
    {
        /* reset the demo ready for the next mode */
        _scan_count = 0;
        _demo_duration.stop();
        _demo_duration.reset();

        /* cycle through all demo modes */
        _set_index++;

        /* switch between advertising and scanning when we go
         * through all the params in the array */
        if (_set_index >= (_is_in_scanning_mode? SCAN_PARAM_SET_MAX : ADV_PARAM_SET_MAX)) {
            _set_index = 0;
            _is_in_scanning_mode = !_is_in_scanning_mode;
        }

        _ble.shutdown();
        _event_queue.break_dispatch();
    }

    /** print some information about our radio activity */
    void print_performance()
    {
        /* measure time from mode start, may have been stopped by timeout */
        uint16_t duration = _demo_duration.read_ms();

        if (_is_in_scanning_mode) {
            /* convert ms into timeslots for accurate calculation as internally
             * all durations are in timeslots (0.625ms) */
            uint16_t interval_ts = GapScanningParams::MSEC_TO_SCAN_DURATION_UNITS(
                scanning_params[_set_index].interval
            );
            uint16_t window_ts = GapScanningParams::MSEC_TO_SCAN_DURATION_UNITS(
                scanning_params[_set_index].window
            );
            uint16_t duration_ts = GapScanningParams::MSEC_TO_SCAN_DURATION_UNITS(
                duration
            );
            /* this is how long we scanned for in timeslots */
            uint16_t rx_ts = (duration_ts / interval_ts) * window_ts;
            /* convert to milliseconds */
            uint16_t rx_ms = (rx_ts * GapScanningParams::UNIT_0_625_MS) / 1000;

            printf("We have scanned for %dms with an interval of %d"
                    " timeslot and a window of %d timeslots\r\n",
                    duration, interval_ts, window_ts);

            printf("We have been listening on the radio for at least %dms\r\n", rx_ms);

        } else /* advertising */ {

            /* convert ms into timeslots for accurate calculation as internally
             * all durations are in timeslots (0.625ms) */
            uint16_t interval_ts = GapAdvertisingParams::MSEC_TO_ADVERTISEMENT_DURATION_UNITS(
                advertising_params[_set_index].interval
            );
            uint16_t duration_ts = GapAdvertisingParams::MSEC_TO_ADVERTISEMENT_DURATION_UNITS(
                duration
            );
            /* this is how many times we advertised */
            uint16_t events = duration_ts / interval_ts;

            printf("We have advertised for %dms"
                   " with an interval of %d timeslots\r\n",
                   duration, interval_ts);

            /* non-scannable and non-connectable advertising
             * skips rx events saving on power consumption */
            if (advertising_params[_set_index].adv_type
                == GapAdvertisingParams::ADV_NON_CONNECTABLE_UNDIRECTED) {
                printf("We created at least %d tx events\r\n", events);
            } else {
                printf("We created at least %d tx and rx events\r\n", events);
            }
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

    while (1) {
        demo.run();
        wait_ms(TIME_BETWEEN_MODES_MS);
        printf("\r\nStarting next GAP demo mode\r\n");
    };

    return 0;
}
