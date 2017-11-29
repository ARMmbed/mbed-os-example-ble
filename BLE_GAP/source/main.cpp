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

static const uint8_t DEVICE_NAME[] = "GAP_device";

/* Duration of each mode in milliseconds */
static const size_t DEMO_DURATION_MS      = 5000;

/* Time between each mode in milliseconds */
static const size_t TIME_BETWEEN_MODES_MS = 1000;

/** the entries in this array are used to configure our advertising
 *  parameters for each of the modes we use in our demo */
static const GapAdvertisingParams advertising_params[] = {
    GapAdvertisingParams(
        GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED,
        GapAdvertisingParams::MSEC_TO_ADVERTISEMENT_DURATION_UNITS(500),
        0
    ),
    GapAdvertisingParams(
        GapAdvertisingParams::ADV_SCANNABLE_UNDIRECTED,
        GapAdvertisingParams::MSEC_TO_ADVERTISEMENT_DURATION_UNITS(500),
        0
    ),
    GapAdvertisingParams(
        GapAdvertisingParams::ADV_NON_CONNECTABLE_UNDIRECTED,
        GapAdvertisingParams::MSEC_TO_ADVERTISEMENT_DURATION_UNITS(500),
        0
    )
};

/* when we cycle through all our advertising modes we will move to scanning modes */

/** the entries in this array are used to configure our scanning
 *  parameters for each of the modes we use in our demo */
static const GapScanningParams scanning_params[] = {
    /*             interval window timeout active */
    GapScanningParams(  4,     4,     0,    false),
    GapScanningParams( 80,    40,     0,    false),
    GapScanningParams(160,    40,     0,    true ),
    GapScanningParams(500,    10,     0,    false)
};

/* get number of items in our arrays */
static const size_t SCAN_PARAM_SET_MAX =
        sizeof(scanning_params) / sizeof(GapScanningParams);
static const size_t ADV_PARAM_SET_MAX  =
        sizeof(advertising_params) / sizeof(GapAdvertisingParams);

/* time from one mode start to next, this includes the spacing between the modes */
static const size_t CYCLE_DURATION_MS = TIME_BETWEEN_MODES_MS + DEMO_DURATION_MS;


/** Demonstrate advertising, scanning and connecting
 */
class GAPDevice : private mbed::NonCopyable<GAPDevice>
{
public:
	GAPDevice() :
	    _ble(BLE::Instance()),
	    _led1(LED1, 0),
        /* because we want to start from the first test and
         * next_mode increments it, we set it to be 0 after increment */
        _set_index((size_t)-1),
        _is_scanning(0),
	    _on_demo_timeout_id(0),
	    _scan_count(0) { };

    ~GAPDevice()
    {
        if (_ble.hasInitialized()) {
            _ble.shutdown();
            printf("BLE shutdown.");
        }
    };

    /** Starts BLE interface initialisation */
    void start()
    {
        ble_error_t error;

        if (_ble.hasInitialized()) {
            print_error(error, "Error: the ble instance has already been initialised.\r\n");
            return;
        }

        _ble.onEventsToProcess(makeFunctionPointer(this, &GAPDevice::schedule_ble_events));

        printf("Initialising BLE.\r\n");
        error = _ble.init(this, &GAPDevice::on_init_complete);

        if (error) {
            print_error(error, "Error returned by BLE::init.\r\n");
            return;
        }

        /* just to show we're running */
        _event_queue.call_every(500, this, &GAPDevice::blink);

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
        printf("Ble instance initialised\r\n");

        /* attach callbacks */
        _ble.gap().onConnection(this, &GAPDevice::on_connect);
        _ble.gap().onDisconnection(this, &GAPDevice::on_disconnect);

        /* start our first demo mode */
        _event_queue.call(this, &GAPDevice::next_mode);
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
            print_error(error, "Error during gap.setAdvertisingPayload\r\n");
            return;
        }

        /* set the advertising parameters according to currently selected set */
        _ble.gap().setAdvertisingParams(advertising_params[_set_index]);

        error = _ble.gap().startAdvertising();

        if (error) {
            print_error(error, "Error during gap.startAdvertising.\r\n");
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
        uint16_t interval = scanning_params[_set_index].getInterval();

        /* number of milliseconds we scan for each time we enter the scanning cycle
         * after the interval set above */
        uint16_t window = scanning_params[_set_index].getWindow();

        /* how long to repeat the cycles of scanning in seconds */
        uint16_t timeout = scanning_params[_set_index].getTimeout();

        /* active scanning will send a scan request to any scanable devices that
         * we see advertising */
        bool active = scanning_params[_set_index].getActiveScanning();

        /* set the scanning parameters according to currently selected set */
        error = _ble.gap().setScanParams(interval, window, timeout, active);

        if (error) {
            print_error(error, "Error during gap.setScanParams\r\n");
            return;
        }

        /* start scanning and attach a callback that will handle advertisements
         * and scan requests responses */
        error = _ble.gap().startScan(this, &GAPDevice::on_scan);

        if (error) {
            print_error(error, "Error during gap.startScan\r\n");
            return;
        }

        printf("Scanning started.\r\n");
    };

    /** After a set duration this cycles to the next demo mode
     *  unless a connection happened first */
    void on_demo_timeout()
    {
        /* alloted time has elapsed, move to next demo mode */
        _event_queue.call(this, &GAPDevice::next_mode);
    };

    /** Look at scan payload to find our peer device and connect to it */
    void on_scan(const Gap::AdvertisementCallbackParams_t *params)
    {
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

            /* if the value contains flags connect to the device if it's discoverable */
            if ((type == GapAdvertisingData::FLAGS) &&
                (*value & GapAdvertisingData::LE_GENERAL_DISCOVERABLE)) {
                /*TODO: connection may complete after we call on_demo_timeout */
                _ble.gap().stopScan();
                _ble.gap().connect(params->peerAddr, Gap::ADDR_TYPE_RANDOM_STATIC, NULL, NULL);
                break;
            }
        }

        _scan_count++;
    };

    /** Immediately disconnects */
    void on_connect(const Gap::ConnectionCallbackParams_t *connection_event)
    {
        /* abort timeout as we connected and the demo will end on disconnection */
        _event_queue.cancel(_on_demo_timeout_id);

        /* measure time from scan start (deduct the time between runs) */
        size_t duration = _demo_duration.read_ms() - TIME_BETWEEN_MODES_MS;
        printf("Connected in %d milliseconds.\r\n", duration);

        _event_queue.call_in(2000, &_ble.gap(), &Gap::disconnect,
                             Gap::LOCAL_HOST_TERMINATED_CONNECTION);
    };

    /** Finishes current demo mode and calls next_mode() to progress the demo */
    void on_disconnect(const Gap::DisconnectionCallbackParams_t *event)
    {
        printf("Disconnected.\r\n");

        /* we have successfully disconnected ending the demo, move to next mode */
        _event_queue.call(this, &GAPDevice::next_mode);
    };

    /** Cycles through all demo modes, switches between scanning and advertising */
    void next_mode()
    {
        if (_is_scanning) {
            printf("Scan count: %d\r\n", _scan_count);
        }

        /* reset the demo ready for the next mode */
        _scan_count = 0;
        _demo_duration.stop();
        _demo_duration.reset();
        _ble.gap().stopAdvertising();
        _ble.gap().stopScan();

        /* cycle through all demo modes */
        _set_index++;

        /* switch to other mode when we go through all the params in one mode */
        if (_set_index >= (_is_scanning? SCAN_PARAM_SET_MAX : ADV_PARAM_SET_MAX)) {
            _set_index = 0;
            _is_scanning = !_is_scanning;
        }

        printf(" -- Next GAP demo mode -- \r\n");

        /* space demo runs by delaying the calls */
        if (_is_scanning) {
            _event_queue.call_in(TIME_BETWEEN_MODES_MS, this, &GAPDevice::scan);
        } else {
            _event_queue.call_in(TIME_BETWEEN_MODES_MS, this, &GAPDevice::advertise);
        }

        /* queue up next demo mode */
        _demo_duration.start();
        _on_demo_timeout_id = _event_queue.call_in(
                CYCLE_DURATION_MS, this, &GAPDevice::on_demo_timeout);
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
