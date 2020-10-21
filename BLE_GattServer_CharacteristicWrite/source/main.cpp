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
#include "gatt_server_process.h"

static EventQueue event_queue(/* event count */ 10 * EVENTS_EVENT_SIZE);

class GattServerDemo : ble::Gap::EventHandler {

    const static uint16_t EXAMPLE_SERVICE_UUID         = 0xA000;
    const static uint16_t WRITABLE_CHARACTERISTIC_UUID = 0xA001;

public:
    GattServerDemo()
    {
        const UUID uuid = WRITABLE_CHARACTERISTIC_UUID;
        _writable_characteristic = new ReadWriteGattCharacteristic<uint8_t> (uuid, &_characteristic_value);

        if (!_writable_characteristic) {
            printf("Allocation of ReadWriteGattCharacteristic failed\r\n");
        }
    }

    ~GattServerDemo()
    {
    }

    void start(BLE &ble, events::EventQueue &event_queue)
    {
        const UUID uuid = EXAMPLE_SERVICE_UUID;
        GattCharacteristic* charTable[] = { _writable_characteristic };
        GattService example_service(uuid, charTable, 1);

        ble.gattServer().addService(example_service);

        ble.gattServer().onDataWritten(this, &GattServerDemo::on_data_written);

        printf("\r\nExample service added with UUID 0xA000\r\n");
        printf("Connect and write to characteristic 0xA001\r\n\r\n");
    }

private:
    /**
     * This callback allows the LEDService to receive updates to the ledState Characteristic.
     *
     * @param[in] params Information about the characterisitc being updated.
     */
    void on_data_written(const GattWriteCallbackParams *params)
    {
        if ((params->handle == _writable_characteristic->getValueHandle()) && (params->len == 1)) {
            printf("New characteristic value written: %x\r\n", *(params->data));
        }
    }

private:
    ReadWriteGattCharacteristic<uint8_t> *_writable_characteristic = nullptr;
    uint8_t _characteristic_value = 0;
};

int main()
{
    BLE &ble = BLE::Instance();

    printf("GattServer demo of writable characteristic\r\n");

    GattServerDemo demo;

    /* this process will handle basic setup and advertising for us */
    BLEProcess ble_process(event_queue, ble);

    /* once it's done it will let us continue with our demo*/
    ble_process.on_init(callback(&demo, &GattServerDemo::start));

    ble_process.start();

    return 0;
}
