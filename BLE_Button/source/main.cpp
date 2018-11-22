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
#include "ble/Gap.h"
#include "ButtonService.h"

DigitalOut  led1(LED1, 1);
InterruptIn button(BLE_BUTTON_PIN_NAME);
ButtonService *buttonServicePtr;
const static char DEVICE_NAME[] = "Button";

static EventQueue eventQueue(/* event count */ 10 * EVENTS_EVENT_SIZE);

void printMacAddress()
{
    /* Print out device MAC address to the console*/
    Gap::AddressType_t addr_type;
    Gap::Address_t address;
    BLE::Instance().gap().getAddress(&addr_type, address);
    printf("DEVICE MAC ADDRESS: ");
    for (int i = 5; i >= 1; i--){
        printf("%02x:", address[i]);
    }
    printf("%02x\r\n", address[0]);
}

void buttonPressedCallback(void) {
    eventQueue.call(Callback<void(bool)>(buttonServicePtr, &ButtonService::updateButtonState), true);
}

void buttonReleasedCallback(void) {
    eventQueue.call(Callback<void(bool)>(buttonServicePtr, &ButtonService::updateButtonState), false);
}

class BatteryDemo : ble::Gap::EventHandler {
public:
    BatteryDemo(BLE &ble, events::EventQueue &eventQueue) :
        _ble(ble),
        _event_queue(eventQueue),
        _button_uuid(ButtonService::BUTTON_SERVICE_UUID) { }

    void start() {
        _ble.gap().setEventHandler(this);

        _ble.init(this, &BatteryDemo::on_init_complete);

        _event_queue.call_every(500, this, &BatteryDemo::blinkCallback);

        _event_queue.dispatch_forever();
    }

private:
    /** Callback triggered when the ble initialization process has finished */
    void on_init_complete(BLE::InitializationCompleteCallbackContext *params) {
        if (params->error != BLE_ERROR_NONE) {
            printf("Ble initialization failed.");
            return;
        }

        printMacAddress();

        /* Setup primary service. */

        buttonServicePtr = new ButtonService(_ble, false /* initial value for button pressed */);

        button.fall(buttonPressedCallback);
        button.rise(buttonReleasedCallback);

        start_advertising();
    }

    void start_advertising() {
        /* Create advertising parameters and payload */

        ble::AdvertisingParameters adv_parameters(
            ble::advertising_type_t::ADV_CONNECTABLE_UNDIRECTED,
            ble::adv_interval_t(ble::millisecond_t(1000))
        );

        uint8_t adv_buffer[ble::LEGACY_ADVERTISING_MAX_SIZE];

        ble::AdvertisingDataBuilder adv_data_builder(adv_buffer);

        adv_data_builder.setFlags();
        adv_data_builder.setLocalServiceList(mbed::make_Span(&_button_uuid, 1));
        adv_data_builder.setName(DEVICE_NAME);

        /* Setup advertising */

        ble_error_t error = _ble.gap().setAdvertisingParameters(
            ble::LEGACY_ADVERTISING_HANDLE,
            adv_parameters
        );

        if (error) {
            print_error(err, "_ble.gap().setAdvertisingParameters() failed\r\n");
            return;
        }

        error = _ble.gap().setAdvertisingPayload(
            ble::LEGACY_ADVERTISING_HANDLE,
            adv_data_builder.getAdvertisingData()
        );

        if (error) {
            print_error(err, "_ble.gap().setAdvertisingPayload() failed\r\n");
            return;
        }

        /* Start advertising */

        error = _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);

        if (error) {
            print_error(err, "_ble.gap().startAdvertising() failed\r\n");
            return;
        }
    }

    void blinkCallback(void) {
        led1 = !led1;
    }

private:
    /* Event handler */

    virtual void onDisconnection(const ble::DisconnectionEvent&) {
        _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);
    }

private:
    BLE &_ble;
    events::EventQueue &_event_queue;

    UUID _button_uuid;
};

void scheduleBleEventsProcessing(BLE::OnEventsToProcessCallbackContext* context) {
    eventQueue.call(Callback<void()>(&context->ble, &BLE::processEvents));
}

int main()
{
    BLE &ble = BLE::Instance();
    ble.onEventsToProcess(scheduleBleEventsProcessing);

    BatteryDemo demo(ble, eventQueue);
    demo.start();

    return 0;
}

