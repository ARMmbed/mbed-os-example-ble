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
#include "SecurityManager.h"

/** This example demonstrates all the basic setup required
 *  to advertise, scan and connect to other devices.
 *
 *  It contains a single class that performs both scans and advertisements.
 *
 *  The demonstrations happens in sequence, after each "mode" ends
 *  the demo jumps to the next mode to continue. There are several modes
 *  that show scanning and several showing advertising. These are configured
 *  according to the two arrays containing parameters. During scanning
 *  a connection will be made to a connectable device upon its discovery.
 */

static const uint8_t DEVICE_NAME[] = "SM_device";

/** Demonstrate advertising, scanning and connecting
 */
class SMDevice : private mbed::NonCopyable<SMDevice>,
                 public SecurityManager::SecurityManagerEventHandler
{
public:
    SMDevice(BLE &ble, events::EventQueue &event_queue) :
        _led1(LED1, 0),
        _ble(ble),
        _event_queue(event_queue),
        _handle(0),
        _is_connecting(false) { };

    virtual ~SMDevice()
    {
        if (_ble.hasInitialized()) {
            _ble.shutdown();
        }
    };

    /** Start BLE interface initialisation */
    void run()
    {
        ble_error_t error;

        if (_ble.hasInitialized()) {
            printf("Ble instance already initialised.\r\n");
            return;
        }

        /* this will inform us off all events so we can schedule their handling
         * using our event queue */
        _ble.onEventsToProcess(
            makeFunctionPointer(this, &SMDevice::schedule_ble_events)
        );

        /* handle timeouts, for example when connection attempts fail */
        _ble.gap().onTimeout(
            makeFunctionPointer(this, &SMDevice::on_timeout)
        );

        error = _ble.init(this, &SMDevice::on_init_complete);

        if (error) {
            printf("Error returned by BLE::init.\r\n");
            return;
        }

        /* to show we're running we'll blink every 500ms */
        _event_queue.call_every(500, this, &SMDevice::blink);

        /* this will not return until shutdown */
        _event_queue.dispatch_forever();
    };

    virtual void start() = 0;

    /* event handler functions */

    virtual void pairingRequest(
        connection_handle_t connectionHandle
    ) {
        printf("Pairing requested. Authorising.\r\n");
        _ble.securityManager().acceptPairingRequest(connectionHandle);
    }

    virtual void pairingResult(
        connection_handle_t connectionHandle,
        SecurityManager::SecurityCompletionStatus_t result
    ) {
        printf("paringResult %d\r\n", result);

        _event_queue.call_in(
            5000, &_ble.gap(),
            &Gap::disconnect, _handle, Gap::REMOTE_USER_TERMINATED_CONNECTION
        );
    }

    virtual void linkEncryptionResult(
        connection_handle_t connectionHandle,
        link_encryption_t result
    ) {
        if (result == link_encryption_t::ENCRYPTED) {
            printf("Link ENCRYPTED\r\n");
        } else if (result == link_encryption_t::ENCRYPTED_WITH_MITM) {
            printf("Link ENCRYPTED_WITH_MITM\r\n");
        } else if (result == link_encryption_t::NOT_ENCRYPTED) {
            printf("Link NOT_ENCRYPTED\r\n");
        }
    }

private:
    /** This is called when BLE interface is initialised and starts the first mode */
    void on_init_complete(BLE::InitializationCompleteCallbackContext *event)
    {
        if (event->error) {
            printf("Error during the initialisation\r\n");
            return;
        }

        ble_error_t error = _ble.securityManager().init();
        if (error) {
            printf("Error during init %d\r\n", error);
            return;
        }

        _ble.securityManager().setSecurityManagerEventHandler(this);

        /* print device address */
        Gap::AddressType_t addr_type;
        Gap::Address_t addr;
        _ble.gap().getAddress(&addr_type, addr);
        printf("Device address: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
               addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);

        /* when scanning we want to connect to a peer device so we need to
         * attach callbacks that are used by Gap to notify us of events */
        _ble.gap().onConnection(this, &SMDevice::on_connect);
        _ble.gap().onDisconnection(this, &SMDevice::on_disconnect);


        _event_queue.call_in(500, this, &SMDevice::start);
    };

    /** This is called by Gap to notify the application we connected,
     *  in our case it immediately disconnects */
    virtual void on_connect(const Gap::ConnectionCallbackParams_t *connection_event) = 0;

    /** This is called by Gap to notify the application we disconnected,
     *  in our case it calls demo_mode_end() to progress the demo */
    void on_disconnect(const Gap::DisconnectionCallbackParams_t *event)
    {
        printf("Disconnected %x \r\n", event->reason);
        _event_queue.break_dispatch();
    };

    /** called if timeout is reached during advertising, scanning
     *  or connection initiation */
    void on_timeout(const Gap::TimeoutSource_t source)
    {
        printf("Unexpected timeout\r\n");
        _event_queue.break_dispatch();
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
    DigitalOut _led1;

protected:
    BLE &_ble;
    events::EventQueue &_event_queue;
    ble::connection_handle_t _handle;
    bool _is_connecting;
};

class SMDeviceCentral : public SMDevice {
public:
    SMDeviceCentral(BLE &ble, events::EventQueue &event_queue) : SMDevice(ble, event_queue) {}

    virtual void start()
    {
        /* start scanning and attach a callback that will handle advertisements
         * and scan requests responses */
        ble_error_t error = _ble.gap().startScan(this, &SMDeviceCentral::on_scan);
        printf("Scanning\r\n");
        if (error) {
            printf("Error during Gap::startScan %d\r\n", error);
            return;
        }
    }

    /** Look at scan payload to find a peer device and connect to it */
    void on_scan(const Gap::AdvertisementCallbackParams_t *params)
    {
        /* don't bother with analysing scan result if we're already connecting */
        if (_is_connecting) {
            return;
        }

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
            const uint8_t *value = params->advertisingData + i + 2;

            /* connect to a discoverable device */
            if ((type == GapAdvertisingData::FLAGS)
                && (*value & GapAdvertisingData::LE_GENERAL_DISCOVERABLE)) {

                ble_error_t error = _ble.gap().connect(
                    params->peerAddr, params->addressType,
                    NULL, NULL
                );

                if (error) {
                    printf("Error during Gap::connect %d\r\n", error);
                    return;
                }

                /* we may have already scan events waiting
                 * to be processed so we need to remember
                 * that we are already connecting and ignore them */
                _is_connecting = true;

                return;
            }

            i += record_length;
        }
    };

    /** This is called by Gap to notify the application we connected,
     *  in our case it immediately disconnects */
    virtual void on_connect(const Gap::ConnectionCallbackParams_t *connection_event)
    {
        ble_error_t error;
        _handle = connection_event->handle;
        printf("Connected\r\n");

        error = _ble.securityManager().requestPairing(_handle);
        if (error) {
            printf("Error during SM::requestPairing %d\r\n", error);
            return;
        }
    };
};

class SMDevicePeripheral : public SMDevice {
public:
    SMDevicePeripheral(BLE &ble, events::EventQueue &event_queue) : SMDevice(ble, event_queue) {}

    virtual void start()
    {
        advertise();
    }

    /** Set up and start advertising */
    void advertise()
    {
        ble_error_t error;
        GapAdvertisingData advertising_data;

        /* add advertising flags */
        advertising_data.addFlags(GapAdvertisingData::LE_GENERAL_DISCOVERABLE
                                  | GapAdvertisingData::BREDR_NOT_SUPPORTED);

        /* add device name */
        advertising_data.addData(
            GapAdvertisingData::COMPLETE_LOCAL_NAME,
            DEVICE_NAME,
            sizeof(DEVICE_NAME)
        );

        error = _ble.gap().setAdvertisingPayload(advertising_data);

        if (error) {
            printf("Error during Gap::setAdvertisingPayload\r\n");
            return;
        }

        /* advertise to everyone */
        _ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
        /* how many milliseconds between advertisements, lower interval
         * increases the chances of being seen at the cost of more power */
        _ble.gap().setAdvertisingInterval(20);
        _ble.gap().setAdvertisingTimeout(0);

        error = _ble.gap().startAdvertising();

        if (error) {
            printf("Error during Gap::startAdvertising.\r\n");
            return;
        }

        _ble.securityManager().setPairingRequestAuthorisation(true);
    };

    /** This is called by Gap to notify the application we connected,
     *  in our case it immediately disconnects */
    virtual void on_connect(const Gap::ConnectionCallbackParams_t *connection_event)
    {
        ble_error_t error;
        _handle = connection_event->handle;
        printf("Connected\r\n");

        error = _ble.securityManager().setLinkSecurity(
            _handle,
            SecurityManager::SECURITY_MODE_ENCRYPTION_NO_MITM
        );

        if (error) {
            printf("Error during SM::setLinkSecurity %d\r\n", error);
            return;
        }
    };
};

int main()
{
    BLE& ble = BLE::Instance();
    events::EventQueue queue;

    {
        printf("\r\n CENTRAL \r\n\r\n");
        SMDeviceCentral central(ble, queue);
        central.run();
    }

    {
        printf("\r\n PERIPHERAL \r\n\r\n");
        SMDevicePeripheral peripheral(ble, queue);
        peripheral.run();
    }

    return 0;
}
