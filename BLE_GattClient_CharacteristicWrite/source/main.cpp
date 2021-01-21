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

#include "events/mbed_events.h"
#include "ble/BLE.h"
#include "ble_app.h"
#include "mbed-trace/mbed_trace.h"

/* GATT server needs free functions */
void service_discovery(const DiscoveredService *service);
void characteristic_discovery(const DiscoveredCharacteristic *characteristic);
void discovery_termination(ble::connection_handle_t connectionHandle);
void on_read(const GattReadCallbackParams *response);
void on_write(const GattWriteCallbackParams *response);

class GattClientDemo : public ble::Gap::EventHandler {
    const static uint16_t EXAMPLE_SERVICE_UUID         = 0xA000;
    const static uint16_t WRITABLE_CHARACTERISTIC_UUID = 0xA001;

    friend void service_discovery(const DiscoveredService *service);
    friend void characteristic_discovery(const DiscoveredCharacteristic *characteristic);
    friend void discovery_termination(ble::connection_handle_t connectionHandle);
    friend void on_read(const GattReadCallbackParams *response);
    friend void on_write(const GattWriteCallbackParams *response);

public:
    static GattClientDemo &get_instance() {
        static GattClientDemo instance;
        return instance;
    }

    void start() {
        _ble_app.add_gap_event_handler(this);
        _ble_app.set_target_name("GattServer");

        /* once it's done it will let us continue with our demo by calling on_init */
        _ble_app.start(callback(this, &GattClientDemo::on_init));
        /* the above function does not return until we call _ble_app.stop() somewhere else */
    }

private:
    /** Callback triggered when the ble initialization process has finished */
    void on_init(BLE &ble, events::EventQueue &event_queue) {
        _ble = &ble;
        _event_queue = &event_queue;
        _ble->gattClient().onDataRead(::on_read);
        _ble->gattClient().onDataWritten(::on_write);
    }

    void onConnectionComplete(const ble::ConnectionCompleteEvent &event) {
        printf("We are looking for a service with UUID 0xA000\r\n");
        printf("And a characteristic with UUID 0xA001\r\n");

        _ble->gattClient().onServiceDiscoveryTermination(::discovery_termination);
        _ble->gattClient().launchServiceDiscovery(
            event.getConnectionHandle(),
            ::service_discovery,
            ::characteristic_discovery,
            EXAMPLE_SERVICE_UUID,
            WRITABLE_CHARACTERISTIC_UUID
        );
    }

private:
    void service_discovery(const DiscoveredService *service) {
        if (service->getUUID().shortOrLong() == UUID::UUID_TYPE_SHORT) {
            if (service->getUUID().getShortUUID() == EXAMPLE_SERVICE_UUID) {
                printf("We found the service we were looking for\r\n");
            }
        }
    }

    void characteristic_discovery(const DiscoveredCharacteristic *characteristic) {
        if (characteristic->getUUID().getShortUUID() == WRITABLE_CHARACTERISTIC_UUID) {
            printf("We found the characteristic we were looking for\r\n");
            writable_characteristic = *characteristic;
            writable_characteristic_found = true;
        }
    }

    void discovery_termination(ble::connection_handle_t connectionHandle) {
        if (writable_characteristic_found) {
            writable_characteristic_found = false;
            _event_queue->call([this]{ writable_characteristic.read(); });
        }
    }

    void on_read(const GattReadCallbackParams *response) {
        if (response->handle == writable_characteristic.getValueHandle()) {
            /* increment the value we just read */
            uint8_t value = response->data[0];
            value++;

            /* and write it back */
            writable_characteristic.write(1, &value);

            printf("Written new value of %x\r\n", value);
        }
    }

    void on_write(const GattWriteCallbackParams *response) {
        if (response->handle == writable_characteristic.getValueHandle()) {
            /* this concludes the example, we stop the app running the ble process in the background */
            _ble_app.stop();
        }
    }

private:
    GattClientDemo() {};
    ~GattClientDemo() {};

private:
    /** Simplified BLE application that automatically advertises and scans. It will
     * initialise BLE and run the event queue */
    BLEApp _ble_app;

    BLE *_ble = nullptr;
    events::EventQueue *_event_queue = nullptr;

    DiscoveredCharacteristic writable_characteristic;
    bool writable_characteristic_found = false;
};

/* redirect to demo instance functions */
void service_discovery(const DiscoveredService *service) {
    GattClientDemo::get_instance().service_discovery(service);
}

void characteristic_discovery(const DiscoveredCharacteristic *characteristic) {
    GattClientDemo::get_instance().characteristic_discovery(characteristic);
}

void discovery_termination(ble::connection_handle_t connectionHandle) {
    GattClientDemo::get_instance().discovery_termination(connectionHandle);
}

void on_read(const GattReadCallbackParams *response) {
    GattClientDemo::get_instance().on_read(response);
}

void on_write(const GattWriteCallbackParams *response) {
    GattClientDemo::get_instance().on_write(response);
}

int main()
{
    printf("\r\nGattClient demo of a writable characteristic\r\n");

    mbed_trace_init();

    GattClientDemo &demo = GattClientDemo::get_instance();

    /* this demo will run and sleep for 5 seconds, during which time ble will be shut down */
    while (1) {
        demo.start();
        printf("Sleeping...\r\n");
        ThisThread::sleep_for(5s);
    }

    return 0;
}
