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
#include "demo.h"

#if MBED_CONF_APP_FILESYSTEM_SUPPORT
#include "LittleFileSystem.h"
#include "HeapBlockDevice.h"
#endif //MBED_CONF_APP_FILESYSTEM_SUPPORT

/** This example demonstrates all the basic setup required
 *  for pairing and setting up link security both as a central and peripheral
 *
 *  The example is implemented as two classes, one for the peripheral and one
 *  for central inheriting from a common base. They are run in sequence and
 *  require a peer device to connect to. During the peripheral device demonstration
 *  a peer device is required to connect. In the central device demonstration
 *  this peer device will be scanned for and connected to - therefore it should
 *  be advertising with the same address as when it connected.
 *
 *  During the test output is written on the serial connection to monitor its
 *  progress.
 */

static const uint8_t DEVICE_NAME[] = "SM_device";

/* for demonstration purposes we will store the peer device address
 * of the device that connects to us in the first demonstration
 * so we can use its address to reconnect to it later */
static BLEProtocol::AddressBytes_t peer_address;

/** Base class for both peripheral and central. The same class that provides
 *  the logic for the application also implements the SecurityManagerEventHandler
 *  which is the interface used by the Security Manager to communicate events
 *  back to the applications. You can provide overrides for a selection of events
 *  your application is interested in.
 */
class SMDevice : private mbed::NonCopyable<SMDevice>,
                 public SecurityManager::EventHandler,
                 public ble::Gap::EventHandler
{
public:
    SMDevice(BLE &ble, events::EventQueue &event_queue, BLEProtocol::AddressBytes_t &peer_address) :
        _led1(LED1, 0),
        _ble(ble),
        _event_queue(event_queue),
        _peer_address(peer_address),
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

        /* to show we're running we'll blink every 500ms */
        _event_queue.call_every(500, this, &SMDevice::blink);

        if (_ble.hasInitialized()) {
            printf("Ble instance already initialised.\r\n");
            return;
        }

        /* this will inform us off all events so we can schedule their handling
         * using our event queue */
        _ble.onEventsToProcess(
            makeFunctionPointer(this, &SMDevice::schedule_ble_events)
        );

        /* handle gap events */
        _ble.gap().setEventHandler(this);

        /* handle timeouts, for example when connection attempts fail */
        _ble.gap().onTimeout(
            makeFunctionPointer(this, &SMDevice::on_timeout)
        );

        error = _ble.init(this, &SMDevice::on_init_complete);

        if (error) {
            printf("Error returned by BLE::init.\r\n");
            return;
        }

        /* this will not return until shutdown */
        _event_queue.dispatch_forever();
    };

    /* event handler functions */

    /** Respond to a pairing request. This will be called by the stack
     * when a pairing request arrives and expects the application to
     * call acceptPairingRequest or cancelPairingRequest */
    virtual void pairingRequest(
        ble::connection_handle_t connectionHandle
    ) {
        printf("Pairing requested - authorising\r\n");
        _ble.securityManager().acceptPairingRequest(connectionHandle);
    }

    /** Inform the application of a successful pairing. Terminate the demonstration. */
    virtual void pairingResult(
        ble::connection_handle_t connectionHandle,
        SecurityManager::SecurityCompletionStatus_t result
    ) {
        if (result == SecurityManager::SEC_STATUS_SUCCESS) {
            printf("Pairing successful\r\n");
        } else {
            printf("Pairing failed\r\n");
        }
    }

    /** Inform the application of change in encryption status. This will be
     * communicated through the serial port */
    virtual void linkEncryptionResult(
        ble::connection_handle_t connectionHandle,
        ble::link_encryption_t result
    ) {
        if (result == ble::link_encryption_t::ENCRYPTED) {
            printf("Link ENCRYPTED\r\n");
        } else if (result == ble::link_encryption_t::ENCRYPTED_WITH_MITM) {
            printf("Link ENCRYPTED_WITH_MITM\r\n");
        } else if (result == ble::link_encryption_t::NOT_ENCRYPTED) {
            printf("Link NOT_ENCRYPTED\r\n");
        }

        /* disconnect in 2 s */
        _event_queue.call_in(
            2000, &_ble.gap(),
            &Gap::disconnect, _handle, Gap::REMOTE_USER_TERMINATED_CONNECTION
        );
    }

private:
    /** Override to start chosen activity when initialisation completes */
    virtual void start() = 0;

    /** This is called when BLE interface is initialised and starts the demonstration */
    void on_init_complete(BLE::InitializationCompleteCallbackContext *event)
    {
        ble_error_t error;

        if (event->error) {
            printf("Error during the initialisation\r\n");
            return;
        }

        /* This path will be used to store bonding information but will fallback
         * to storing in memory if file access fails (for example due to lack of a filesystem) */
        const char* db_path = "/fs/bt_sec_db";
        /* If the security manager is required this needs to be called before any
         * calls to the Security manager happen. */
        error = _ble.securityManager().init(
            true,
            false,
            SecurityManager::IO_CAPS_NONE,
            NULL,
            false,
            db_path
        );

        if (error) {
            printf("Error during init %d\r\n", error);
            return;
        }

        error = _ble.securityManager().preserveBondingStateOnReset(true);

        if (error) {
            printf("Error during preserveBondingStateOnReset %d\r\n", error);
        }

#if MBED_CONF_APP_FILESYSTEM_SUPPORT
        /* Enable privacy so we can find the keys */
        error = _ble.gap().enablePrivacy(true);

        if (error) {
            printf("Error enabling privacy\r\n");
        }

        Gap::PeripheralPrivacyConfiguration_t configuration_p = {
            /* use_non_resolvable_random_address */ false,
            Gap::PeripheralPrivacyConfiguration_t::REJECT_NON_RESOLVED_ADDRESS
        };
        _ble.gap().setPeripheralPrivacyConfiguration(&configuration_p);

        Gap::CentralPrivacyConfiguration_t configuration_c = {
            /* use_non_resolvable_random_address */ false,
            Gap::CentralPrivacyConfiguration_t::RESOLVE_AND_FORWARD
        };
        _ble.gap().setCentralPrivacyConfiguration(&configuration_c);

        /* this demo switches between being master and slave */
        _ble.securityManager().setHintFutureRoleReversal(true);
#endif

        /* Tell the security manager to use methods in this class to inform us
         * of any events. Class needs to implement SecurityManagerEventHandler. */
        _ble.securityManager().setSecurityManagerEventHandler(this);

        /* print device address */
        Gap::AddressType_t addr_type;
        Gap::Address_t addr;
        _ble.gap().getAddress(&addr_type, addr);
        printf("Device address: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
               addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);

        /* start test in 500 ms */
        _event_queue.call_in(500, this, &SMDevice::start);
    };

    /** This is called by Gap to notify the application we disconnected,
     *  in our case it ends the demonstration. */
    virtual void onDisconnectionComplete(const ble::DisconnectionEvent &) {
    {
        printf("Diconnected\r\n");
        _event_queue.break_dispatch();
    };

    /** End demonstration unexpectedly. Called if timeout is reached during advertising,
     * scanning or connection initiation */
    void on_timeout(const Gap::TimeoutSource_t source)
    {
        printf("Unexpected timeout - aborting\r\n");
        _event_queue.break_dispatch();
    };

    /** Schedule processing of events from the BLE in the event queue. */
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
    BLEProtocol::AddressBytes_t &_peer_address;
    ble::connection_handle_t _handle;
    bool _is_connecting;
};

/** A peripheral device will advertise, accept the connection and request
 * a change in link security. */
class SMDevicePeripheral : public SMDevice {
public:
    SMDevicePeripheral(BLE &ble, events::EventQueue &event_queue, BLEProtocol::AddressBytes_t &peer_address)
        : SMDevice(ble, event_queue, peer_address) { }

    virtual void start()
    {
        /* Set up and start advertising */

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

        printf("Please connect to device\r\n");

        /** This tells the stack to generate a pairingRequest event
         * which will require this application to respond before pairing
         * can proceed. Setting it to false will automatically accept
         * pairing. */
        _ble.securityManager().setPairingRequestAuthorisation(true);
    };

    /** This is called by Gap to notify the application we connected,
     *  in our case it immediately requests a change in link security */
    virtual void onConnectionComplete(const ble::ConnectionCompleteEvent &event)
    {
        ble_error_t error;

        /* remember the device that connects to us now so we can connect to it
         * during the next demonstration */
        memcpy(_peer_address, connection_event->peerAddr, sizeof(_peer_address));

        printf("Connected to peer: ");
        print_address(event.getPeerAddress().data());

        _handle = event.getConnectionHandle();

        /* Request a change in link security. This will be done
         * indirectly by asking the master of the connection to
         * change it. Depending on circumstances different actions
         * may be taken by the master which will trigger events
         * which the applications should deal with. */
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

/** A central device will scan, connect to a peer and request pairing. */
class SMDeviceCentral : public SMDevice {
public:
    SMDeviceCentral(BLE &ble, events::EventQueue &event_queue, BLEProtocol::AddressBytes_t &peer_address)
        : SMDevice(ble, event_queue, peer_address) { }

    virtual void start()
    {
        /* start scanning and attach a callback that will handle advertisements
         * and scan requests responses */
        ble_error_t error = _ble.gap().startScan(this, &SMDeviceCentral::on_scan);

        printf("Please advertise\r\n");

        printf("Scanning for: %02x:%02x:%02x:%02x:%02x:%02x\r\n",
               _peer_address[5], _peer_address[4], _peer_address[3],
               _peer_address[2], _peer_address[1], _peer_address[0]);

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

        /* connect to the same device that connected to us */
        if (memcmp(params->peerAddr, _peer_address, sizeof(_peer_address)) == 0) {

            ble_error_t error = _ble.gap().connect(
                params->peerAddr, params->peerAddrType,
                NULL, NULL
            );

            if (error) {
                printf("Error during Gap::connect %d\r\n", error);
                return;
            }

            printf("Connecting... ");

            /* we may have already scan events waiting
             * to be processed so we need to remember
             * that we are already connecting and ignore them */
            _is_connecting = true;

            return;
        }
    };

    /** This is called by Gap to notify the application we connected,
     *  in our case it immediately request pairing */
    virtual void on_connect(const Gap::ConnectionCallbackParams_t *connection_event)
    {
        ble_error_t error;

        /* store the handle for future Security Manager requests */
        _handle = connection_event->handle;

        /* in this example the local device is the master so we request pairing */
        error = _ble.securityManager().requestPairing(_handle);

        printf("Connected\r\n");

        if (error) {
            printf("Error during SM::requestPairing %d\r\n", error);
            return;
        }

        /* upon pairing success the application will disconnect */
    };
};


#if MBED_CONF_APP_FILESYSTEM_SUPPORT
bool create_filesystem()
{
    static LittleFileSystem fs("fs");

    /* replace this with any physical block device your board supports (like an SD card) */
    static HeapBlockDevice bd(4096, 256);

    int err = bd.init();

    if (err) {
        return false;
    }

    err = bd.erase(0, bd.size());

    if (err) {
        return false;
    }

    err = fs.mount(&bd);

    if (err) {
        /* Reformat if we can't mount the filesystem */
        printf("No filesystem found, formatting...\r\n");

        err = fs.reformat(&bd);

        if (err) {
            return false;
        }
    }

    return true;
}
#endif //MBED_CONF_APP_FILESYSTEM_SUPPORT

int main()
{
    BLE& ble = BLE::Instance();
    events::EventQueue queue;

#if MBED_CONF_APP_FILESYSTEM_SUPPORT
    /* if filesystem creation fails or there is no filesystem the security manager
     * will fallback to storing the security database in memory */
    if (!create_filesystem()) {
        printf("Filesystem creation failed, will use memory storage\r\n");
    }
#endif

    while(1) {
        {
            printf("\r\n PERIPHERAL \r\n\r\n");
            SMDevicePeripheral peripheral(ble, queue, peer_address);
            peripheral.run();
        }

        {
            printf("\r\n CENTRAL \r\n\r\n");
            SMDeviceCentral central(ble, queue, peer_address);
            central.run();
        }
    }

    return 0;
}
