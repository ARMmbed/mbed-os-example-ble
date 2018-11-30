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
#include "pretty_printer.h"

static const char DEVICE_NAME[] = "GAPButton";

static EventQueue event_queue(/* event count */ 16 * EVENTS_EVENT_SIZE);

/** Demonstrate advertising, scanning and connecting
 */
class GapButtonDemo : private mbed::NonCopyable<GapButtonDemo>, public ble::Gap::EventHandler
{
public:
    GapButtonDemo(BLE& ble, events::EventQueue& event_queue) :
        _ble(ble),
        _event_queue(event_queue),
        _led1(LED1, 0),
        /* We can arbiturarily choose the GAPButton service UUID to be 0xAA00
         * as long as it does not overlap with the UUIDs defined here:
         * https://developer.bluetooth.org/gatt/services/Pages/ServicesHome.aspx */
        _button_uuid(0xAA00),
        _button(BLE_BUTTON_PIN_NAME),
        _adv_data_builder(_adv_buffer) { }

    ~GapButtonDemo()
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

        ble_error_t error = _ble.init(this, &GapButtonDemo::on_init_complete);

        if (error) {
            printf("Error returned by BLE::init.\r\n");
            return;
        }

        /* to show we're running we'll blink every 500ms */
        _event_queue.call_every(500, this, &GapButtonDemo::blink);

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

        print_mac_address();

        _button.rise(Callback<void()>(this, &GapButtonDemo::button_pressed_callback));

        start_advertising();
    }

    void start_advertising() {
        /* Create advertising parameters and payload */

        ble::AdvertisingParameters adv_parameters(
            ble::advertising_type_t::CONNECTABLE_UNDIRECTED,
            ble::adv_interval_t(ble::millisecond_t(1000))
        );

        _adv_data_builder.setFlags();
        _adv_data_builder.setLocalServiceList(mbed::make_Span(&_button_uuid, 1));
        _adv_data_builder.setName(DEVICE_NAME);

        update_button_payload();

        /* Setup advertising */

        ble_error_t error = _ble.gap().setAdvertisingParameters(
            ble::LEGACY_ADVERTISING_HANDLE,
            adv_parameters
        );

        if (error) {
            print_error(error, "_ble.gap().setAdvertisingParameters() failed");
            return;
        }

        error = _ble.gap().setAdvertisingPayload(
            ble::LEGACY_ADVERTISING_HANDLE,
            _adv_data_builder.getAdvertisingData()
        );

        if (error) {
            print_error(error, "_ble.gap().setAdvertisingPayload() failed");
            return;
        }

        /* Start advertising */

        error = _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);

        if (error) {
            print_error(error, "_ble.gap().startAdvertising() failed");
            return;
        }
    }

    void update_button_payload()
    {
        /* The Service Data data type consists of a service UUID with the data associated with that service. */
        ble_error_t error = _adv_data_builder.setServiceData(
            _button_uuid,
            mbed::make_Span(&_button_count, 1)
        );

        if (error != BLE_ERROR_NONE) {
            print_error(error, "Updating payload failed");
        }
    }

    void button_pressed_callback()
    {
        ++_button_count;

        /* Calling BLE api in interrupt context may cause race conditions
           Using mbed-events to schedule calls to BLE api for safety */
        _event_queue.call(Callback<void()>(this, &GapButtonDemo::update_button_payload));
    }

    /** Blink LED to show we're running */
    void blink(void)
    {
        _led1 = !_led1;
    }

private:
    /* Event handler */

    void onDisconnectionComplete(const ble::DisconnectionEvent&) {
        _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);
    }

private:
    BLE                &_ble;
    events::EventQueue &_event_queue;

    DigitalOut  _led1;
    const UUID  _button_uuid;

    InterruptIn _button;
    uint8_t     _button_count;

    uint8_t     _adv_buffer[ble::LEGACY_ADVERTISING_MAX_SIZE];
    ble::AdvertisingDataBuilder _adv_data_builder;
};

/** Schedule processing of events from the BLE middleware in the event queue. */
void schedule_ble_events(BLE::OnEventsToProcessCallbackContext *context) {
    event_queue.call(Callback<void()>(&context->ble, &BLE::processEvents));
}

int main()
{
    BLE &ble = BLE::Instance();

    /* this will inform us of all events so we can schedule their handling
     * using our event queue */
    ble.onEventsToProcess(schedule_ble_events);

    GapButtonDemo demo(ble, event_queue);

    demo.run();

    return 0;
}
