/* mbed Microcontroller Library
 * Copyright (c) 2006-2014 ARM Limited
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
#include "ble/services/BatteryService.h"

static DigitalOut led1(LED1, 1);

const static char DEVICE_NAME[] = "BATTERY";

static events::EventQueue eventQueue(/* event count */ 16 * EVENTS_EVENT_SIZE);

void printMacAddress() {
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

class BatteryDemo : ble::Gap::EventHandler {
public:
    BatteryDemo(BLE &ble, events::EventQueue &eventQueue) :
        _ble(ble),
        _eventQueue(eventQueue),
        _batteryUUID(GattService::UUID_BATTERY_SERVICE),
        _batteryLevel(50),
        _batteryService(ble, _batteryLevel) { }

    void start() {
        _ble.gap().setEventHandler(this);

        _ble.init(this, &BatteryDemo::initComplete);

        _eventQueue.call_every(500, this, &BatteryDemo::blinkCallback);
        _eventQueue.call_every(1000, this, &BatteryDemo::updateSensorValue);

        _eventQueue.dispatch_forever();
    }

private:
    /** Callback triggered when the ble initialization process has finished */
    void initComplete(BLE::InitializationCompleteCallbackContext *params) {
        if (params->error != BLE_ERROR_NONE) {
            printf("Ble initialization failed.");
            return;
        }

        printMacAddress();

        /* Create advertising parameters and payload */

        ble::AdvertisingParameters adv_parameters(
            ble::advertising_type_t::ADV_CONNECTABLE_UNDIRECTED,
            ble::adv_interval_t(ble::millisecond_t(1000))
        );

        uint8_t adv_buffer[ble::LEGACY_ADVERTISING_MAX_SIZE];

        ble::AdvertisingDataBuilder adv_data_builder(adv_buffer);

        adv_data_builder.setFlags();
        adv_data_builder.setLocalServiceList(mbed::make_Span(&_batteryUUID, 1));
        adv_data_builder.setName(DEVICE_NAME);

        /* Setup advertising */

        _ble.gap().setAdvertisingParameters(
            ble::LEGACY_ADVERTISING_HANDLE,
            adv_parameters
        );

        _ble.gap().setAdvertisingPayload(
            ble::LEGACY_ADVERTISING_HANDLE,
            adv_data_builder.getAdvertisingData()
        );

        /* Start advertising */

        _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);
    }

    void updateSensorValue() {
        if (_ble.gap().getState().connected) {
            _batteryLevel++;
            if (_batteryLevel > 100) {
                _batteryLevel = 20;
            }

            _batteryService.updateBatteryLevel(_batteryLevel);
        }
    }

    void blinkCallback(void) {
        led1 = !led1;
    }

private:
    /* Event handler */

    void onDisconnection(const ble::DisconnectionEvent&) {
        _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);
    }

private:
    BLE &_ble;
    events::EventQueue &_eventQueue;

    UUID _batteryUUID;

    uint8_t _batteryLevel;
    BatteryService _batteryService;
};

void scheduleBleEventsProcessing(BLE::OnEventsToProcessCallbackContext* context) {
    BLE &ble = BLE::Instance();
    eventQueue.call(Callback<void()>(&ble, &BLE::processEvents));
}

int main()
{
    BLE &ble = BLE::Instance();
    ble.onEventsToProcess(scheduleBleEventsProcessing);

    BatteryDemo demo(ble, eventQueue);
    demo.start();

    return 0;
}
