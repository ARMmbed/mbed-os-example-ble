/* mbed Microcontroller Library
 * Copyright (c) 2017 ARM Limited
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

#ifndef GATT_SERVER_EXAMPLE_BLE_PROCESS_H_
#define GATT_SERVER_EXAMPLE_BLE_PROCESS_H_

#include <stdint.h>
#include <stdio.h>

#include "events/EventQueue.h"
#include "platform/Callback.h"
#include "platform/NonCopyable.h"

#include "ble/BLE.h"
#include "ble/Gap.h"
#include "ble/GapAdvertisingParams.h"
#include "ble/GapAdvertisingData.h"
#include "ble/FunctionPointerWithContext.h"

/**
 * Handle initialization adn shutdown of the BLE Instance.
 *
 * Setup advertising payload and manage advertising state.
 * Delegate to GattClientProcess once the connection is established.
 */
class BLEProcess : private mbed::NonCopyable<BLEProcess> {
public:
    /**
     * Construct a BLEProcess from an event queue and a ble interface.
     *
     * Call start() to initiate ble processing.
     */
    BLEProcess(events::EventQueue &event_queue, BLE &ble_interface) :
        _event_queue(event_queue),
        _ble_interface(ble_interface),
        _post_init_cb() {
    }

    ~BLEProcess()
    {
        stop();
    }

   /**
     * Subscription to the ble interface initialization event.
     *
     * @param[in] cb The callback object that will be called when the ble
     * interface is initialized.
     */
    void on_init(mbed::Callback<void(BLE&, events::EventQueue&)> cb)
    {
        _post_init_cb = cb;
    }

    /**
     * Initialize the ble interface, configure it and start advertising.
     */
    bool start()
    {
        printf("Ble process started.\r\n");

        if (_ble_interface.hasInitialized()) {
            printf("Error: the ble instance has already been initialized.\r\n");
            return false;
        }

        _ble_interface.onEventsToProcess(
            makeFunctionPointer(this, &BLEProcess::schedule_ble_events)
        );

        ble_error_t error = _ble_interface.init(
            this, &BLEProcess::when_init_complete
        );

        if (error) {
            printf("Error: %u returned by BLE::init.\r\n", error);
            return false;
        }

        return true;
    }

    /**
     * Close existing connections and stop the process.
     */
    void stop()
    {
        if (_ble_interface.hasInitialized()) {
            _ble_interface.shutdown();
            printf("Ble process stopped.");
        }
    }

private:

    /**
     * Schedule processing of events from the BLE middleware in the event queue.
     */
    void schedule_ble_events(BLE::OnEventsToProcessCallbackContext *event)
    {
        _event_queue.call(mbed::callback(&event->ble, &BLE::processEvents));
    }

    /**
     * Sets up adverting payload and start advertising.
     *
     * This function is invoked when the ble interface is initialized.
     */
    void when_init_complete(BLE::InitializationCompleteCallbackContext *event)
    {
        if (event->error) {
            printf("Error %u during the initialization\r\n", event->error);
            return;
        }
        printf("Ble instance initialized\r\n");

        Gap &gap = _ble_interface.gap();
        gap.onConnection(this, &BLEProcess::when_connection);
        gap.onDisconnection(this, &BLEProcess::when_disconnection);

        if (!set_advertising_parameters()) {
            return;
        }

        if (!set_advertising_data()) {
            return;
        }

        if (!start_advertising()) {
            return;
        }

        if (_post_init_cb) {
            _post_init_cb(_ble_interface, _event_queue);
        }
    }

    void when_connection(const Gap::ConnectionCallbackParams_t *connection_event)
    {
        printf("Connected.\r\n");
    }

    void when_disconnection(const Gap::DisconnectionCallbackParams_t *event)
    {
        printf("Disconnected.\r\n");
        start_advertising();
    }

    bool start_advertising(void)
    {
        Gap &gap = _ble_interface.gap();

        /* Start advertising the set */
        ble_error_t error = gap.startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);

        if (error) {
            printf("Error %u during gap.startAdvertising.\r\n", error);
            return false;
        } else {
            printf("Advertising started.\r\n");
            return true;
        }
    }

    bool set_advertising_parameters()
    {
        Gap &gap = _ble_interface.gap();

        ble_error_t error = gap.setAdvertisingParameters(
            ble::LEGACY_ADVERTISING_HANDLE,
            ble::AdvertisingParameters()
        );

        if (error) {
            printf("Gap::setAdvertisingParameters() failed with error %d", error);
            return false;
        }

        return true;
    }

    bool set_advertising_data()
    {
        Gap &gap = _ble_interface.gap();

        /* Use the simple builder to construct the payload; it fails at runtime
         * if there is not enough space left in the buffer */
        ble_error_t error = gap.setAdvertisingPayload(
            ble::LEGACY_ADVERTISING_HANDLE,
            ble::AdvertisingDataSimpleBuilder<ble::LEGACY_ADVERTISING_MAX_SIZE>()
                .setFlags()
                .setName("GattServer")
                .getAdvertisingData()
        );

        if (error) {
            printf("Gap::setAdvertisingPayload() failed with error %d", error);
            return false;
        }

        return true;
    }

    events::EventQueue &_event_queue;
    BLE &_ble_interface;
    mbed::Callback<void(BLE&, events::EventQueue&)> _post_init_cb;
};

#endif /* GATT_SERVER_EXAMPLE_BLE_PROCESS_H_ */
