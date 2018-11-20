/* mbed Microcontroller Library
 * Copyright (c) 2006-2015 ARM Limited
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
#include "ble/services/iBeacon.h"

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

class BeaconDemo : ble::Gap::EventHandler {
public:
    BeaconDemo(BLE &ble, events::EventQueue &eventQueue) :
        _ble(ble),
        _eventQueue(eventQueue) { }

    void start() {
        _ble.gap().setEventHandler(this);

        _ble.init(this, &BeaconDemo::initComplete);

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

        /**
         * The Beacon payload has the following composition:
         * 128-Bit / 16byte UUID = E2 0A 39 F4 73 F5 4B C4 A1 2F 17 D1 AD 07 A9 61
         * Major/Minor  = 0x1122 / 0x3344
         * Tx Power     = 0xC8 = 200, 2's compliment is 256-200 = (-56dB)
         *
         * Note: please remember to calibrate your beacons TX Power for more accurate results.
         */
        static const uint8_t uuid[] = { 0xE2, 0x0A, 0x39, 0xF4, 0x73, 0xF5, 0x4B, 0xC4,
                                        0xA1, 0x2F, 0x17, 0xD1, 0xAD, 0x07, 0xA9, 0x61 };
        uint16_t majorNumber = 1122;
        uint16_t minorNumber = 3344;
        uint16_t txPower     = 0xC8;

        _ibeaconPtr = new iBeacon(_ble, uuid, majorNumber, minorNumber, txPower);

        ble::AdvertisingParameters adv_parameters(
            ble::advertising_type_t::ADV_CONNECTABLE_UNDIRECTED,
            ble::adv_interval_t(ble::millisecond_t(1000))
        );

        _ble.gap().setAdvertisingParameters(
            ble::LEGACY_ADVERTISING_HANDLE,
            adv_parameters
        );

        _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);
    }

private:
    /* Event handler */

    void onDisconnection(const ble::DisconnectionEvent&) {
        _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);
    }

private:
    BLE &_ble;
    events::EventQueue &_eventQueue;

    iBeacon* _ibeaconPtr;
};

void scheduleBleEventsProcessing(BLE::OnEventsToProcessCallbackContext* context) {
    BLE &ble = BLE::Instance();
    eventQueue.call(Callback<void()>(&ble, &BLE::processEvents));
}

int main()
{
    BLE &ble = BLE::Instance();
    ble.onEventsToProcess(scheduleBleEventsProcessing);

    BeaconDemo demo(ble, eventQueue);
    demo.start();

    return 0;
}
