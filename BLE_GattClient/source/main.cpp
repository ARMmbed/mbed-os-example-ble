
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

#include <memory>
#include <new>
#include <stdio.h>

#include "events/EventQueue.h"
#include "platform/NonCopyable.h"

#include "ble/BLE.h"
#include "ble/Gap.h"
#include "ble/GattClient.h"
#include "ble/GapAdvertisingParams.h"
#include "ble/GapAdvertisingData.h"
#include "ble/GattClient.h"
#include "ble/DiscoveredService.h"
#include "ble/DiscoveredCharacteristic.h"
#include "ble/CharacteristicDescriptorDiscovery.h"

#include "BLEProcess.h"

/**
 * Handle discovery of the GATT server.
 *
 * First the GATT server is discovered in its entirety then each readable
 * characteristic is read and the client register to characteristic
 * notifications or indication when available. The client report server
 * indications and notification until the connection end.
 */
class GattClientProcess : private mbed::NonCopyable<GattClientProcess>,
                          public ble::Gap::EventHandler {

    // Internal typedef to this class type.
    // It is used as a shorthand to pass member function as callbacks.
    typedef GattClientProcess Self;

    typedef CharacteristicDescriptorDiscovery::DiscoveryCallbackParams_t
        DiscoveryCallbackParams_t;

    typedef CharacteristicDescriptorDiscovery::TerminationCallbackParams_t
        TerminationCallbackParams_t;

    typedef DiscoveredCharacteristic::Properties_t Properties_t;

public:

    /**
     * Construct an empty client process.
     *
     * The function start() shall be called to initiate the discovery process.
     */
    GattClientProcess() :
        _client(NULL),
        _connection_handle(),
        _characteristics(NULL),
        _it(NULL),
        _descriptor_handle(0),
        _ble_interface(NULL),
        _event_queue(NULL) {
    }

    ~GattClientProcess()
    {
        stop();
    }

    void init(BLE &ble_interface, events::EventQueue &event_queue)
    {
        _ble_interface = &ble_interface;
        _event_queue = &event_queue;
        _client = &_ble_interface->gattClient();

        _ble_interface->gap().setEventHandler(this);
    }

    /**
     * Start the discovery process.
     *
     * @param[in] client The GattClient instance which will discover the distant
     * GATT server.
     * @param[in] connection_handle Reference of the connection to the GATT
     * server which will be discovered.
     */
    void start()
    {
        // setup the event handlers called during the process
        _client->onDataWritten().add(as_cb(&Self::when_descriptor_written));
        _client->onHVX().add(as_cb(&Self::when_characteristic_changed));

        // The discovery process will invoke when_service_discovered when a
        // service is discovered, when_characteristic_discovered when a
        // characteristic is discovered and when_service_discovery_ends once the
        // discovery process has ended.
        _client->onServiceDiscoveryTermination(as_cb(&Self::when_service_discovery_ends));
        ble_error_t error = _client->launchServiceDiscovery(
            _connection_handle,
            as_cb(&Self::when_service_discovered),
            as_cb(&Self::when_characteristic_discovered)
        );

        if (error) {
            printf("Error %u returned by _client->launchServiceDiscovery.\r\n", error);
            return;
        }

        printf("Client process started: initiate service discovery.\r\n");
    }

    /**
     * Stop the discovery process and clean the instance.
     */
    void stop()
    {
        if (!_client) {
            return;
        }

        // unregister event handlers
        _client->onDataWritten().detach(as_cb(&Self::when_descriptor_written));
        _client->onHVX().detach(as_cb(&Self::when_characteristic_changed));
        _client->onServiceDiscoveryTermination(NULL);

        // remove discovered characteristics
        clear_characteristics();

        // clean up the instance
        _connection_handle = 0;
        _characteristics = NULL;
        _it = NULL;
        _descriptor_handle = 0;

        printf("Client process stopped.\r\n");
    }

private:
    /**
     * Event handler invoked when a connection is established.
     *
     * This function setup the connection handle to operate on then start the
     * discovery process.
     */
    void onConnectionComplete(const ble::ConnectionCompleteEvent &event)
    {
        _connection_handle = event.connectionHandle;
        _event_queue->call(mbed::callback(this, &Self::start));
    }

    /**
     * Stop the discovery process and clean the instance.
     */
    void onDisconnection(const ble::DisconnectionEvent &event)
    {
        if (_client && event.connectionHandle == _connection_handle) {
            stop();
        }
    }

private:
////////////////////////////////////////////////////////////////////////////////
// Service and characteristic discovery process.

    /**
     * Handle services discovered.
     *
     * The GattClient invokes this function when a service has been discovered.
     *
     * @see GattClient::launchServiceDiscovery
     */
    void when_service_discovered(const DiscoveredService *discovered_service)
    {
        // print information of the service discovered
        printf("Service discovered: value = ");
        print_uuid(discovered_service->getUUID());
        printf(", start = %u, end = %u.\r\n",
            discovered_service->getStartHandle(),
            discovered_service->getEndHandle()
        );
    }

    /**
     * Handle characteristics discovered.
     *
     * The GattClient invoke this function when a characteristic has been
     * discovered.
     *
     * @see GattClient::launchServiceDiscovery
     */
    void when_characteristic_discovered(const DiscoveredCharacteristic *discovered_characteristic)
    {
        // print characteristics properties
        printf("\tCharacteristic discovered: uuid = ");
        print_uuid(discovered_characteristic->getUUID());
        printf(", properties = ");
        print_properties(discovered_characteristic->getProperties());
        printf(
            ", decl handle = %u, value handle = %u, last handle = %u.\r\n",
            discovered_characteristic->getDeclHandle(),
            discovered_characteristic->getValueHandle(),
            discovered_characteristic->getLastHandle()
        );

        // add the characteristic into the list of discovered characteristics
        bool success = add_characteristic(discovered_characteristic);
        if (!success) {
            printf("Error: memory allocation failure while adding the discovered characteristic.\r\n");
            _client->terminateServiceDiscovery();
            stop();
            return;
        }
    }

    /**
     * Handle termination of the service and characteristic discovery process.
     *
     * The GattClient invokes this function when the service and characteristic
     * discovery process ends.
     *
     * @see GattClient::onServiceDiscoveryTermination
     */
    void when_service_discovery_ends(Gap::Handle_t connection_handle)
    {
        if (!_characteristics) {
            printf("No characteristics discovered, end of the process.\r\n");
            return;
        }

        printf("All services and characteristics discovered, process them.\r\n");

        // reset iterator and start processing characteristics in order
        _it = NULL;
        _event_queue->call(mbed::callback(this, &Self::process_next_characteristic));
    }

////////////////////////////////////////////////////////////////////////////////
// Processing of characteristics based on their properties.

    /**
     * Process the characteristics discovered.
     *
     * - If the characteristic is readable then read its value and print it. Then
     * - If the characteristic can emit notification or indication then discover
     * the characteristic CCCD and subscribe to the server initiated event.
     * - Otherwise skip the characteristic processing.
     */
    void process_next_characteristic(void)
    {
        if (!_it) {
            _it = _characteristics;
        } else {
            _it = _it->next;
        }

        while (_it) {
            Properties_t properties = _it->value.getProperties();

            if (properties.read()) {
                read_characteristic(_it->value);
                return;
            } else if(properties.notify() || properties.indicate()) {
                discover_descriptors(_it->value);
                return;
            } else {
                printf(
                    "Skip processing of characteristic %u\r\n",
                    _it->value.getValueHandle()
                );
                _it = _it->next;
            }
        }

        printf("All characteristics discovered have been processed.\r\n");
    }

    /**
     * Initate the read of the characteristic in input.
     *
     * The completion of the operation will happens in when_characteristic_read()
     */
    void read_characteristic(const DiscoveredCharacteristic &characteristic)
    {
        printf("Initiating read at %u.\r\n", characteristic.getValueHandle());
        ble_error_t error = characteristic.read(
            0, as_cb(&Self::when_characteristic_read)
        );

        if (error) {
            printf(
                "Error: cannot initiate read at %u due to %u\r\n",
                characteristic.getValueHandle(), error
            );
            stop();
        }
    }

    /**
     * Handle the reception of a read response.
     *
     * If the characteristic can emit notification or indication then start the
     * discovery of the the characteristic descriptors then subscribe to the
     * server initiated event by writing the CCCD discovered. Otherwise start
     * the processing of the next characteristic discovered in the server.
     */
    void when_characteristic_read(const GattReadCallbackParams *read_event)
    {
        printf("\tCharacteristic value at %u equal to: ", read_event->handle);
        for (size_t i = 0; i <  read_event->len; ++i) {
            printf("0x%02X ", read_event->data[i]);
        }
        printf(".\r\n");

        Properties_t properties = _it->value.getProperties();

        if(properties.notify() || properties.indicate()) {
            discover_descriptors(_it->value);
        } else {
            process_next_characteristic();
        }
    }

    /**
     * Initiate the discovery of the descriptors of the characteristic in input.
     *
     * When a descriptor is discovered, the function when_descriptor_discovered
     * is invoked.
     */
    void discover_descriptors(const DiscoveredCharacteristic &characteristic)
    {
        printf("Initiating descriptor discovery of %u.\r\n", characteristic.getValueHandle());

        _descriptor_handle = 0;
        ble_error_t error = characteristic.discoverDescriptors(
            as_cb(&Self::when_descriptor_discovered),
            as_cb(&Self::when_descriptor_discovery_ends)
        );

        if (error) {
            printf(
                "Error: cannot initiate discovery of %04X due to %u.\r\n",
                characteristic.getValueHandle(), error
            );
            stop();
        }
    }

    /**
     * Handle the discovery of the characteristic descriptors.
     *
     * If the descriptor found is a CCCD then stop the discovery. Once the
     * process has ended subscribe to server initiated events by writing the
     * value of the CCCD.
     */
    void when_descriptor_discovered(const DiscoveryCallbackParams_t* event)
    {
        printf("\tDescriptor discovered at %u, UUID: ", event->descriptor.getAttributeHandle());
        print_uuid(event->descriptor.getUUID());
        printf(".\r\n");

        if (event->descriptor.getUUID() == BLE_UUID_DESCRIPTOR_CLIENT_CHAR_CONFIG) {
            _descriptor_handle = event->descriptor.getAttributeHandle();
            _client->terminateCharacteristicDescriptorDiscovery(
                event->characteristic
            );
        }
    }

    /**
     * If a CCCD has been found subscribe to server initiated events by writing
     * its value.
     */
    void when_descriptor_discovery_ends(const TerminationCallbackParams_t *event) {
        // shall never happen but happen with android devices ...
        // process the next charateristic
        if (!_descriptor_handle) {
            printf("\tWarning: characteristic with notify or indicate attribute without CCCD.\r\n");
            process_next_characteristic();
            return;
        }

        Properties_t properties = _it->value.getProperties();

        uint16_t cccd_value =
            (properties.notify() << 0) | (properties.indicate() << 1);

        ble_error_t error = _client->write(
            GattClient::GATT_OP_WRITE_REQ,
            _connection_handle,
            _descriptor_handle,
            sizeof(cccd_value),
            reinterpret_cast<uint8_t*>(&cccd_value)
        );

        if (error) {
            printf(
                "Error: cannot initiate write of CCCD %u due to %u.\r\n",
                _descriptor_handle, error
            );
            stop();
        }
    }

    /**
     * Called when the CCCD has been written.
     */
    void when_descriptor_written(const GattWriteCallbackParams* event)
    {
        // should never happen
        if (!_descriptor_handle) {
            printf("\tError: received write response to unsolicited request.\r\n");
            stop();
            return;
        }

        printf("\tCCCD at %u written.\r\n", _descriptor_handle);
        _descriptor_handle = 0;
        process_next_characteristic();
    }

    /**
     * Print the updated value of the characteristic.
     *
     * This function is called when the server emits a notification or an
     * indication of a characteristic value the client has subscribed to.
     *
     * @see GattClient::onHVX()
     */
    void when_characteristic_changed(const GattHVXCallbackParams* event)
    {
        printf("Change on attribute %u: new value = ", event->handle);
        for (size_t i = 0; i < event->len; ++i) {
            printf("0x%02X ", event->data[i]);
        }
        printf(".\r\n");
    }

    struct DiscoveredCharacteristicNode {
        DiscoveredCharacteristicNode(const DiscoveredCharacteristic &c) :
            value(c), next(NULL) { }

        DiscoveredCharacteristic value;
        DiscoveredCharacteristicNode *next;
    };

    /**
     * Add a discovered characteristic into the list of discovered characteristics.
     */
    bool add_characteristic(const DiscoveredCharacteristic *characteristic)
    {
        DiscoveredCharacteristicNode* new_node =
            new(std::nothrow) DiscoveredCharacteristicNode(*characteristic);

        if (new_node == false) {
            printf("Error while allocating a new characteristic.\r\n");
            return false;
        }

        if (_characteristics == NULL) {
            _characteristics = new_node;
        } else {
            DiscoveredCharacteristicNode* c = _characteristics;
            while(c->next) {
                c = c->next;
            }
            c->next = new_node;
        }

        return true;
    }

    /**
     * Clear the list of discovered characteristics.
     */
    void clear_characteristics(void)
    {
        DiscoveredCharacteristicNode *c= _characteristics;

        while (c) {
            DiscoveredCharacteristicNode *n = c->next;
            delete c;
            c = n;
        }
    }

    /**
     * Helper to construct an event handler from a member function of this
     * instance.
     */
    template<typename ContextType>
    FunctionPointerWithContext<ContextType> as_cb(
        void (Self::*member)(ContextType context)
    ) {
        return makeFunctionPointer(this, member);
    }

    /**
     * Print the value of a UUID.
     */
    static void print_uuid(const UUID &uuid)
    {
        const uint8_t *uuid_value = uuid.getBaseUUID();

        // UUIDs are in little endian, print them in big endian
        for (size_t i = 0; i < uuid.getLen(); ++i) {
            printf("%02X", uuid_value[(uuid.getLen() - 1) - i]);
        }
    }

    /**
     * Print the value of a characteristic properties.
     */
    static void print_properties(const Properties_t &properties)
    {
        const struct {
            bool (Properties_t::*fn)() const;
            const char* str;
        } prop_to_str[] = {
            { &Properties_t::broadcast, "broadcast" },
            { &Properties_t::read, "read" },
            { &Properties_t::writeWoResp, "writeWoResp" },
            { &Properties_t::write, "write" },
            { &Properties_t::notify, "notify" },
            { &Properties_t::indicate, "indicate" },
            { &Properties_t::authSignedWrite, "authSignedWrite" }
        };

        printf("[");
        for (size_t i = 0; i < (sizeof(prop_to_str) / sizeof(prop_to_str[0])); ++i) {
            if ((properties.*(prop_to_str[i].fn))()) {
                printf(" %s", prop_to_str[i].str);
            }
        }
        printf(" ]");
    }

    GattClient *_client;
    Gap::Handle_t _connection_handle;
    DiscoveredCharacteristicNode *_characteristics;
    DiscoveredCharacteristicNode *_it;
    GattAttribute::Handle_t _descriptor_handle;
    BLE *_ble_interface;
    events::EventQueue *_event_queue;
};


int main() {

    BLE &ble_interface = BLE::Instance();
    events::EventQueue event_queue;
    BLEProcess ble_process(event_queue, ble_interface);
    GattClientProcess gatt_client_process;

    // Register GattClientProcess::init in the ble_process; this function will
    // be called once the ble_interface is initialized.
    ble_process.on_init(
        mbed::callback(&gatt_client_process, &GattClientProcess::init)
    );

    // bind the event queue to the ble interface, initialize the interface
    // and start advertising
    ble_process.start();

    // Process the event queue.
    event_queue.dispatch_forever();

    return 0;
}
