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
#include "print_error.h"

/** This example demonstrates all the basic setup required
 *  to advertise, scan and connect to other devices.
 *
 *  It contains a single class that performs both scans and advertisements.
 *
 *  The demonstrations happens in sequence, after each "mode" ends
 *  the demo jumps to the next mode to continue. The "modes" are described
 *  in the two arrays containing the parameters for scanning and advertising.
 */

static const uint8_t DEVICE_NAME[]        = "GAP_device";

/* Duration of each mode in milliseconds */
static const size_t MODE_DURATION_MS      = 5000;

/* Time between each mode in milliseconds */
static const size_t TIME_BETWEEN_MODES_MS = 1000;

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
    /*            advertising type                      interval timeout */
    { GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED,      10,    3000 },
    { GapAdvertisingParams::ADV_SCANNABLE_UNDIRECTED,        50,    0    },
    { GapAdvertisingParams::ADV_NON_CONNECTABLE_UNDIRECTED,  50,    0    }
};

/* when we cycle through all our advertising modes we will move to scanning modes */

/** the entries in this array are used to configure our scanning
 *  parameters for each of the modes we use in our demo */
static const ScanModeParam_t scanning_params[] = {
    /* interval window timeout active */
    {   4,     4,     0,    false },
    {  80,    40,     0,    false },
    { 160,    40,     0,    true  },
    { 500,    10,     0,    false }
};

/* get number of items in our arrays */
static const size_t SCAN_PARAM_SET_MAX =
        sizeof(scanning_params) / sizeof(GapScanningParams);
static const size_t ADV_PARAM_SET_MAX  =
        sizeof(advertising_params) / sizeof(GapAdvertisingParams);


/** Demonstrate advertising, scanning and connecting
 */
class GAPDevice : private mbed::NonCopyable<GAPDevice>
{
public:
    GAPDevice() :
        _ble(BLE::Instance()),
        _led1(LED1, 0),
        _set_index(0),
        _is_scanning(0),
        _on_demo_timeout_id(0),
        _scan_count(0) { };

    ~GAPDevice()
    {
        if (_ble.hasInitialized()) {
            _ble.shutdown();
        }
    };

    /** Start BLE interface initialisation */
    void start()
    {
        ble_error_t error;

        if (_ble.hasInitialized()) {
            print_error(error, "Ble instance already initialised.\r\n");
            return;
        }

        _ble.onEventsToProcess(
                makeFunctionPointer(this, &GAPDevice::schedule_ble_events));

        error = _ble.init(this, &GAPDevice::on_init_complete);

        if (error) {
            print_error(error, "Error returned by BLE::init.\r\n");
            return;
        }

        /* to show we're running we'll blink every 500ms */
        _event_queue.call_every(500, this, &GAPDevice::blink);

        /* this will not return until shutdown */
        _event_queue.dispatch_forever();
    };

private:
    /** This is called when BLE interface is initialised and starts the first mode */
    void on_init_complete(BLE::InitializationCompleteCallbackContext *event)
    {
        if (event->error) {
            print_error(event->error, "Error during the initialisation\r\n");
            return;
        }

        /* start our first demo mode,
         * all calls are serialised on the user thread through the event queue */
        _event_queue.call(this, &GAPDevice::demo_mode_start);
    };

    /** Set up and start advertising */
    void advertise()
    {
        ble_error_t error;
        GapAdvertisingData advertising_data;

        /* add advertising flags */
        advertising_data.addFlags(GapAdvertisingData::LE_GENERAL_DISCOVERABLE |
                                  GapAdvertisingData::BREDR_NOT_SUPPORTED);

        /* add device name */
        advertising_data.addData(GapAdvertisingData::COMPLETE_LOCAL_NAME,
                                 DEVICE_NAME,
                                 sizeof(DEVICE_NAME));

        error = _ble.gap().setAdvertisingPayload(advertising_data);

        if (error) {
            print_error(error, "Error during Gap::setAdvertisingPayload\r\n");
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
            print_error(error, "Error during Gap::startAdvertising.\r\n");
            return;
        }

        printf("Advertising started.\r\n");
    };

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
            print_error(error, "Error during Gap::setScanParams\r\n");
            return;
        }

        /* start scanning and attach a callback that will handle advertisements
         * and scan requests responses */
        error = _ble.gap().startScan(this, &GAPDevice::on_scan);

        if (error) {
            print_error(error, "Error during Gap::startScan\r\n");
            return;
        }

        printf("Scanning started.\r\n");
    };

    /** After a set duration this cycles to the next demo mode
     *  unless a connection happened first */
    void on_demo_timeout()
    {
        if (_is_scanning) {
            printf("Failed to connect after scanning %d advertisements\r\n", _scan_count);
        }

        print_performance();

        /* alloted time has elapsed, move to next demo mode */
        _event_queue.call(this, &GAPDevice::demo_mode_end);
    };

    /** Look at scan payload to find a peer device and connect to it */
    void on_scan(const Gap::AdvertisementCallbackParams_t *params)
    {
        _scan_count++;

        /* parse the advertising payload, looking for a discoverable device */
        for (uint8_t i = 0; i < params->advertisingDataLen; ++i) {
            /* The advertising payload is a collection of key/value records where
             * byte 0: length of the record excluding this byte
             * byte 1: The key, it is the type of the data
             * byte [2..N] The value. N is equal to byte0 - 1 */
            const uint8_t record_length = params->advertisingData[i];
            if (record_length == 0) {
                continue;
            }
            const uint8_t type = params->advertisingData[i + 1];
            const uint8_t* value = params->advertisingData + i + 2;

            /* connect to a discoverable device */
            if ((type == GapAdvertisingData::FLAGS)
                && (*value & GapAdvertisingData::LE_GENERAL_DISCOVERABLE)) {

                ble_error_t error = _ble.gap().connect(
                        params->peerAddr, Gap::ADDR_TYPE_RANDOM_STATIC, NULL, NULL);

                if (error) {
                    print_error(error, "Error during Gap::connect\r\n");
                    return;
                }

                /* abort timeout as the mode will end on disconnection */
                _event_queue.cancel(_on_demo_timeout_id);
                /* start a new timeout in case we fail to connect */
                _on_demo_timeout_id = _event_queue.call_in(
                        MODE_DURATION_MS, this, &GAPDevice::demo_mode_end);

                return;
            }
        }
    };

    /** This is called by Gap to notify the application we connected,
     *  in our case it immediately disconnects */
    void on_connect(const Gap::ConnectionCallbackParams_t *connection_event)
    {
        print_performance();

        printf("Connected in %dms\r\n", _demo_duration.read_ms());

        /* cancel the connect timeout since we connected */
        _event_queue.cancel(_on_demo_timeout_id);

        _event_queue.call_in(2000, &_ble.gap(), &Gap::disconnect,
                             Gap::LOCAL_HOST_TERMINATED_CONNECTION);
    };

    /** This is called by Gap to notify the application we disconnected,
     *  in our case it calls demo_mode_end() to progress the demo */
    void on_disconnect(const Gap::DisconnectionCallbackParams_t *event)
    {
        printf("Disconnected\r\n");

        /* we have successfully disconnected ending the demo, move to next mode */
        _event_queue.call(this, &GAPDevice::demo_mode_end);
    };

    /** clean up after last run, cycle to the next mode and launch it */
    void demo_mode_end()
    {
        /* reset the demo ready for the next mode */
        _scan_count = 0;
        _demo_duration.stop();
        _demo_duration.reset();
        /* this stops scanning, advertising and drops connections */
        _ble.gap().reset();

        /* cycle through all demo modes */
        _set_index++;

        /* switch between advertising and scanning when we go
         * through all the params in the array */
        if (_set_index >= (_is_scanning? SCAN_PARAM_SET_MAX : ADV_PARAM_SET_MAX)) {
            _set_index = 0;
            _is_scanning = !_is_scanning;
        }

        printf("\r\n -- Next GAP demo mode -- \r\n");

        _event_queue.call_in(TIME_BETWEEN_MODES_MS, this, &GAPDevice::demo_mode_start);
    };

    /** queue up start of the demo mode */
    void demo_mode_start()
    {
        /* space demo runs by delaying the calls */
        if (_is_scanning) {
            /* when scanning we want to connect to a peer device so we need to
             * attach callbacks that are used by Gap to notify us of events */
            _ble.gap().onConnection(this, &GAPDevice::on_connect);
            _ble.gap().onDisconnection(this, &GAPDevice::on_disconnect);

            _event_queue.call(this, &GAPDevice::scan);
        } else {
            _event_queue.call(this, &GAPDevice::advertise);
        }

        /* for performance measurement keep track of duration of the demo mode */
        _demo_duration.start();
        /* queue up next demo mode */
        _on_demo_timeout_id = _event_queue.call_in(
                MODE_DURATION_MS, this, &GAPDevice::on_demo_timeout);

        printf("\r\n");
    }

    /** print some information about our radio activity */
    void print_performance()
    {
        /* measure time from mode start, we may have to cap it by timeout */
        uint16_t duration = _demo_duration.read_ms();

        if (_is_scanning) {
            /* timeout stop us early */
            if (scanning_params[_set_index].timeout
                && duration > scanning_params[_set_index].timeout) {
                duration = scanning_params[_set_index].timeout;
            }

            /* convert ms into timeslots for accurate calculation as internally
             * all durations are in timeslots (0.625ms) */
            uint16_t interval_ts = GapScanningParams::MSEC_TO_SCAN_DURATION_UNITS(
                    scanning_params[_set_index].interval);
            uint16_t window_ts = GapScanningParams::MSEC_TO_SCAN_DURATION_UNITS(
                    scanning_params[_set_index].window);
            uint16_t duration_ts = GapScanningParams::MSEC_TO_SCAN_DURATION_UNITS(
                    duration);
            /* this is how long we scanned for in timeslots */
            uint16_t rx_ts = (duration / interval_ts) * window_ts;
            /* convert to milliseconds */
            uint16_t rx_ms = (rx_ts * GapScanningParams::UNIT_0_625_MS) / 1000;

            printf("We have scanned for %dms\r\n", duration);
            printf("with an interval of %d timeslots \r\n", interval_ts);
            printf("and a window of %d timeslots \r\n", window_ts);

            printf("We have been listening on the radio for at least %dms\r\n", rx_ms);
        } else /* advertising */ {
            /* timeout stop us early */
            if (scanning_params[_set_index].timeout
                && duration > advertising_params[_set_index].timeout) {
                duration = advertising_params[_set_index].timeout;
            }

            /* convert ms into timeslots for accurate calculation as internally
             * all durations are in timeslots (0.625ms) */
            uint16_t interval_ts = GapAdvertisingParams::MSEC_TO_ADVERTISEMENT_DURATION_UNITS(
                    advertising_params[_set_index].interval);
            uint16_t duration_ts = GapAdvertisingParams::MSEC_TO_ADVERTISEMENT_DURATION_UNITS(
                    duration);
            /* this is how many times we advertised */
            uint16_t events = duration / interval_ts;

            printf("We have advertised for %dms\r\n", duration);
            printf("with an interval of %d timeslots \r\n", interval_ts);

            /* non-scannable and non-connectable advertising skips rx events saving us power*/
            if (advertising_params[_set_index].adv_type
                    == GapAdvertisingParams::ADV_NON_CONNECTABLE_UNDIRECTED) {
                printf("We created at least %d tx events\r\n", events);
            } else {
                printf("We created at least %d tx and rx events\r\n", events);
            }
        }
    };

    /** Schedule processing of events from the BLE middleware in the event queue. */
    void schedule_ble_events(BLE::OnEventsToProcessCallbackContext *context)
    {
        _event_queue.call(mbed::callback(&context->ble, &BLE::processEvents));
    };

    /** Blink LED to show we're running */
    void blink(void)
    {
        _led1 = !_led1;
    };

private:
    BLE                &_ble;
    events::EventQueue  _event_queue;
    DigitalOut          _led1;

    /* Keep track of our progress through demo modes */
    size_t              _set_index;
    bool                _is_scanning;

    /* Remember the call id of the function on _event_queue
     * that handles timeouts so we can cancel it */
    int                 _on_demo_timeout_id;

    /* Measure performance of our advertising/scanning */
    Timer               _demo_duration;
    size_t              _scan_count;
};

int main()
{
    GAPDevice gap_device;

    gap_device.start();

    return 0;
}
