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


static const uint8_t DEVICE_NAME[]    = "GAP_device";
static const uint8_t PEER_NAME[]      = "MotoE2(4G-LTE)";

/* Duration of each mode in milliseconds */
static const size_t DEMO_DURATION_MS      = 5000;
/* Time between each mode in milliseconds */
static const size_t TIME_BETWEEN_MODES_MS = 1000;


static const GapScanningParams scanning_params[] = {
    GapScanningParams(GapScanningParams::SCAN_INTERVAL_MIN,
                      GapScanningParams::SCAN_WINDOW_MIN, 0),
    GapScanningParams(GapScanningParams::SCAN_INTERVAL_MIN,
                      GapScanningParams::SCAN_WINDOW_MIN, 0),
    GapScanningParams(GapScanningParams::SCAN_INTERVAL_MIN,
                      GapScanningParams::SCAN_WINDOW_MIN, 0),
    GapScanningParams(GapScanningParams::SCAN_INTERVAL_MIN,
                      GapScanningParams::SCAN_WINDOW_MIN, 0)
};

static const GapAdvertisingParams advertising_params[] = {
    GapAdvertisingParams(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED,
                         GapAdvertisingParams::MSEC_TO_ADVERTISEMENT_DURATION_UNITS(500),
                         0),
    GapAdvertisingParams(GapAdvertisingParams::ADV_CONNECTABLE_DIRECTED,
                         GapAdvertisingParams::MSEC_TO_ADVERTISEMENT_DURATION_UNITS(500),
                         0),
    GapAdvertisingParams(GapAdvertisingParams::ADV_SCANNABLE_UNDIRECTED,
                         GapAdvertisingParams::MSEC_TO_ADVERTISEMENT_DURATION_UNITS(500),
                         0),
    GapAdvertisingParams(GapAdvertisingParams::ADV_NON_CONNECTABLE_UNDIRECTED,
                         GapAdvertisingParams::MSEC_TO_ADVERTISEMENT_DURATION_UNITS(500),
                         0)
};

static const size_t SCAN_PARAM_SET_MAX = sizeof(scanning_params) / sizeof(GapScanningParams);
static const size_t ADV_PARAM_SET_MAX  = sizeof(advertising_params) / sizeof(GapAdvertisingParams);
/* This includes the spacing between the modes */
static const us_timestamp_t CYCLE_DURATION_US = (TIME_BETWEEN_MODES_MS + DEMO_DURATION_MS) * 1000;

static void print_error(ble_error_t error, const char* msg)
{
    printf("%s", msg);
    switch(error) {
        case BLE_ERROR_NONE:
            printf("BLE_ERROR_NONE: No error");
            break;
        case BLE_ERROR_BUFFER_OVERFLOW:
            printf("BLE_ERROR_BUFFER_OVERFLOW: The requested action would cause a buffer overflow and has been aborted");
            break;
        case BLE_ERROR_NOT_IMPLEMENTED:
            printf("BLE_ERROR_NOT_IMPLEMENTED: Requested a feature that isn't yet implement or isn't supported by the target HW");
            break;
        case BLE_ERROR_PARAM_OUT_OF_RANGE:
            printf("BLE_ERROR_PARAM_OUT_OF_RANGE: One of the supplied parameters is outside the valid range");
            break;
        case BLE_ERROR_INVALID_PARAM:
            printf("BLE_ERROR_INVALID_PARAM: One of the supplied parameters is invalid");
            break;
        case BLE_STACK_BUSY:
            printf("BLE_STACK_BUSY: The stack is busy");
            break;
        case BLE_ERROR_INVALID_STATE:
            printf("BLE_ERROR_INVALID_STATE: Invalid state");
            break;
        case BLE_ERROR_NO_MEM:
            printf("BLE_ERROR_NO_MEM: Out of Memory");
            break;
        case BLE_ERROR_OPERATION_NOT_PERMITTED:
            printf("BLE_ERROR_OPERATION_NOT_PERMITTED");
            break;
        case BLE_ERROR_INITIALIZATION_INCOMPLETE:
            printf("BLE_ERROR_INITIALIZATION_INCOMPLETE");
            break;
        case BLE_ERROR_ALREADY_INITIALIZED:
            printf("BLE_ERROR_ALREADY_INITIALIZED");
            break;
        case BLE_ERROR_UNSPECIFIED:
            printf("BLE_ERROR_UNSPECIFIED: Unknown error");
            break;
        case BLE_ERROR_INTERNAL_STACK_FAILURE:
            printf("BLE_ERROR_INTERNAL_STACK_FAILURE: internal stack failure");
            break;
    }
    printf("\r\n");
}

/** Demonstrate advertising, scanning and connecting
 */
class GAPDevice : private mbed::NonCopyable<GAPDevice>
{
public:
	GAPDevice(BLE &ble) : _ble(ble), _led1(LED1, 0), _set_index(size_t(-1)),
	    _is_scanning(0), _radio_count(0) { };
    ~GAPDevice();

    /** Starts BLE interface initialisation */
    void start();

private:
    /** This is called when BLE interface is initialised and starts the main loop */
    void on_init_complete(BLE::InitializationCompleteCallbackContext *event);

    /** Set up and start advertising */
    void advertise();
    /** Set up and start scanning */
    void scan();

    /** After a set duration this cycles to the next demo mode unless a connection happened first */
    void on_demo_timeout();
    /** Look at scan payload to find our peer device and connect to it */
    void on_scan(const Gap::AdvertisementCallbackParams_t *params);
    /** Immediately disconnects */
    void on_connect(const Gap::ConnectionCallbackParams_t *connection_event);
    /** Finishes current demo mode and calls next_mode() to progress the demo */
    void on_disconnect(const Gap::DisconnectionCallbackParams_t *event);

    /** Cycles through all demo modes, switches between scanning and advertising */
    void next_mode();

    /** Help us keep track of power consumption approximation */
    void on_radio(bool active);
    /** Schedule processing of events from the BLE middleware in the event queue. */
    void schedule_ble_events(BLE::OnEventsToProcessCallbackContext *event);
    /** Blink LED to show we're running */
    void blink(void);

private:
    BLE                &_ble;
    events::EventQueue  _event_queue;
    DigitalOut          _led1;

    /* Keep track of our progress through demo modes */
    size_t              _set_index;
    bool                _is_scanning;

    /* Measure performance of our advertising/scanning */
    Timer               _demo_duration;
    Ticker              _timeout_ticker;
    size_t              _radio_count;
};

void GAPDevice::start()
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
}

void GAPDevice::on_init_complete(BLE::InitializationCompleteCallbackContext *event)
{
    if (event->error) {
        print_error(event->error, "Error during the initialisation\r\n");
        return;
    }
    printf("Ble instance initialised\r\n");

    /* attach callbacks */
    _ble.gap().onConnection(this, &GAPDevice::on_connect);
    _ble.gap().onDisconnection(this, &GAPDevice::on_disconnect);
    _ble.gap().onRadioNotification(this, &GAPDevice::on_radio);

    /* start our first demo mode */
    _event_queue.call(this, &GAPDevice::next_mode);
}

void GAPDevice::advertise()
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
}

void GAPDevice::on_demo_timeout()
{
    /* alloted time has elapsed, move to next demo */
    _event_queue.call(this, &GAPDevice::next_mode);
}

void GAPDevice::scan()
{
    ble_error_t error;

    /* set the scanning parameters according to currently selected set */
    error = _ble.gap().setScanParams(scanning_params[_set_index].getInterval(),
                                     scanning_params[_set_index].getWindow(),
                                     scanning_params[_set_index].getTimeout(),
                                     scanning_params[_set_index].getActiveScanning());

    if (error) {
        print_error(error, "Error during gap.setScanParams\r\n");
        return;
    }

    /* start scanning and attach a callback that will handle advertisments and scan requests responses */
    error = _ble.gap().startScan(this, &GAPDevice::on_scan);

    if (error) {
        print_error(error, "Error during gap.startScan\r\n");
        return;
    }

    printf("Scanning started.\r\n");
}

void GAPDevice::on_scan(const Gap::AdvertisementCallbackParams_t *params)
{
    /* we want to find our peer device based on the advertising data and connect to it*/

    for (uint8_t i = 0; i < params->advertisingDataLen; ++i) {
        /* parse the advertising payload, looking for data type COMPLETE_LOCAL_NAME
           The advertising payload is a collection of key/value records where
           byte 0: length of the record excluding this byte
           byte 1: The key, it is the type of the data
           byte [2..N] The value. N is equal to byte0 - 1 */
        const uint8_t record_length = params->advertisingData[i];
        if (record_length == 0) {
            continue;
        }
        const uint8_t type = params->advertisingData[i + 1];
        const uint8_t* value = params->advertisingData + i + 2;
        const uint8_t value_length = record_length - 1;

        /* if the data matches the set PEER_NAME connect to that device */
        if ((type == GapAdvertisingData::COMPLETE_LOCAL_NAME) &&
            (value_length == sizeof(PEER_NAME)) &&
            (memcmp(value, PEER_NAME, value_length) == 0)) {
            printf("adv peerAddr[%02x %02x %02x %02x %02x %02x] rssi %d, isScanResponse %u, AdvertisementType %u\r\n",
                   params->peerAddr[5], params->peerAddr[4], params->peerAddr[3], params->peerAddr[2],
                   params->peerAddr[1], params->peerAddr[0], params->rssi, params->isScanResponse, params->type);
            /*TODO: connection may complete after we call on_demo_timeout */
            _ble.gap().connect(params->peerAddr, Gap::ADDR_TYPE_RANDOM_STATIC, NULL, NULL);
            break;
        }
        i += record_length;
    }
}

void GAPDevice::on_connect(const Gap::ConnectionCallbackParams_t *connection_event)
{
    /* abort the timeout since we connected and the demo will end by disconnection */
    _timeout_ticker.detach();

    /* measure time from scan start (deduct the time between runs) */
    size_t duration = _demo_duration.read_ms() - TIME_BETWEEN_MODES_MS;
    printf("Connected in %d milliseconds.\r\n", duration);

    _ble.gap().disconnect(Gap::LOCAL_HOST_TERMINATED_CONNECTION);
}

void GAPDevice::on_disconnect(const Gap::DisconnectionCallbackParams_t *event)
{
    printf("Disconnected.\r\n");

    /* we have successfully disconnected thus ending the demo, move to next demo */
    _event_queue.call(this, &GAPDevice::next_mode);
}

void GAPDevice::next_mode()
{
    printf("Radio count: %d\r\n", _radio_count);

    /* reset the demo ready for the next mode */
    _radio_count = 0;
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
    _timeout_ticker.attach_us(mbed::callback(this, &GAPDevice::on_demo_timeout), CYCLE_DURATION_US);
}

void GAPDevice::on_radio(bool active)
{
    if (active) {
        _radio_count++;
    }
}

void GAPDevice::blink(void)
{
    _led1 = !_led1;
}

void GAPDevice::schedule_ble_events(BLE::OnEventsToProcessCallbackContext *context)
{
    _event_queue.call(mbed::callback(&context->ble, &BLE::processEvents));
}

GAPDevice::~GAPDevice()
{
    if (_ble.hasInitialized()) {
        _ble.shutdown();
        printf("BLE shutdown.");
    }
}

int main()
{
    BLE &ble_interface = BLE::Instance();
    GAPDevice gap_device(ble_interface);

    gap_device.start();

    return 0;
}
