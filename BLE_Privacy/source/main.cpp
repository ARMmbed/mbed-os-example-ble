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

/** This example demonstrates privacy features in Gap. It shows how to use
 *  private addresses when advertising and connecting and how filtering ties
 *  in with these operations.
 *
 *  The application will start by repeatedly trying to connect to the same
 *  application running on another board. It will do this by advertising and
 *  scanning for random intervals waiting until the difference in intervals
 *  between the boards will make them meet when one is advertising and the
 *  other scanning.
 *
 *  Two devices will be operating using random resolvable addresses. The
 *  applications will connect to the peer and pair. It will attempt bonding
 *  and if possible create a whitelist based on the bond. Subsequent connections
 *  will turn on filtering if the whitelist has been successfully created.
 */

static const uint8_t DEVICE_NAME[] = "Privacy";

/** print device address to the terminal */
void print_address(const Gap::Address_t &addr)
{
    printf("%02x:%02x:%02x:%02x:%02x:%02x\r\n",
           addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
}

/** Base class for both peripheral and central. The same class that provides
 *  the logic for the application also implements the SecurityManagerEventHandler
 *  which is the interface used by the Security Manager to communicate events
 *  back to the applications. You can provide overrides for a selection of events
 *  your application is interested in.
 */
class PrivacyDevice : private mbed::NonCopyable<PrivacyDevice>,
                      public SecurityManager::EventHandler
{
public:
    PrivacyDevice(BLE &ble, events::EventQueue &event_queue) :
        _ble(ble),
        _event_queue(event_queue),
        _handle(0),
        _bonded(false),
        _whitelist_generated(false),
        _led1(LED1, 0) { };

    virtual ~PrivacyDevice()
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
        _event_queue.call_every(500, this, &PrivacyDevice::blink);

        if (_ble.hasInitialized()) {
            printf("Ble instance already initialised.\r\n");
            return;
        }

        /* this will inform us off all events so we can schedule their handling
         * using our event queue */
        _ble.onEventsToProcess(
            makeFunctionPointer(this, &PrivacyDevice::schedule_ble_events)
        );

        error = _ble.init(this, &PrivacyDevice::on_init_complete);

        if (error) {
            printf("Error returned by BLE::init.\r\n");
            return;
        }

        /* this will not return until shutdown */
        _event_queue.dispatch_forever();
    };

    /** Override to start chosen activity when initialisation completes */
    virtual void start() = 0;

    /** Override to start chosen activity after initial bonding */
    virtual void start_after_bonding() = 0;

    /* event handler functions */

    /** Inform the application of pairing. When succesful we will attempt
     *  to create a whitelist based on the newly created bond */
    virtual void pairingResult(
        ble::connection_handle_t connectionHandle,
        SecurityManager::SecurityCompletionStatus_t result
    ) {
        if (result == SecurityManager::SEC_STATUS_SUCCESS) {
            printf("Pairing successful\r\n");

            if (!_bonded) {
                /* generate whitelist only the first time we bond */
                _bonded = true;

                /* provide memory for the generated whitelist */
                Gap::Whitelist_t* whitelist = new Gap::Whitelist_t;
                whitelist->size = 0;
                /* we only need space for one address in our demonstration
                 * but this can return all the bonded addresses */
                whitelist->capacity = 1;
                whitelist->addresses = new BLEProtocol::Address_t();

                /* this will only fill the whitelist up to the provided capacity,
                 * we hand over the memory ownership to the function */
                ble_error_t error = _ble.securityManager().generateWhitelistFromBondTable(whitelist);

                if (error != BLE_ERROR_NONE) {
                    delete whitelist->addresses;
                    delete whitelist;
                }
            }
        } else {
            printf("Pairing failed\r\n");
        }

        /* disconnect in 1000 ms */
        _event_queue.call_in(
            1000, &_ble.gap(),
            &Gap::disconnect, _handle, Gap::REMOTE_USER_TERMINATED_CONNECTION
        );
    }


    virtual void whitelistFromBondTable(Gap::Whitelist_t* whitelist)
    {
        if (whitelist->size) {
            printf("Whitelist generated.\r\n");
            _whitelist_generated = true;

            /* set the newly created whitelist at the link layer,
             * see BLUETOOTH SPECIFICATION Version 5.0 | Vol 6, Part B - 4.3 */
            _ble.gap().setWhitelist(*whitelist);
        } else {
            printf("Whitelist failed to generate.\r\n");
        }

        /* this callback transfer memory ownership to us so we have to dispose of it */
        delete whitelist->addresses;
        delete whitelist;
    }

    /* callbacks */

    /** This is called when BLE interface is initialised and starts the demonstration */
    void on_init_complete(BLE::InitializationCompleteCallbackContext *event)
    {
        ble_error_t error;

        if (event->error) {
            printf("Error during the initialisation\r\n");
            _event_queue.break_dispatch();
            return;
        }

        /* for use by tools we print out own address and also use it
         * to seed RNG as the address is unique */
        print_local_address();

        /* when scanning we want to connect to a peer device so we need to
         * attach callbacks that are used by Gap to notify us of events */
        _ble.gap().onConnection(this, &PrivacyDevice::on_connect);
        _ble.gap().onDisconnection(this, &PrivacyDevice::on_disconnect);
        _ble.gap().onTimeout(makeFunctionPointer(this, &PrivacyDevice::on_timeout));

        /* Privacy requires the security manager */

        error = _ble.securityManager().init(
            /* enableBonding */ true,
            /* requireMITM */ false,
            /* iocaps */ SecurityManager::IO_CAPS_NONE,
            /* passkey */ NULL,
            /* signing */ false,
            /* dbFilepath */ NULL
        );

        if (error) {
            printf("Error during security manager initialisation\r\n");
            _event_queue.break_dispatch();
            return;
        }

        /* Tell the security manager to use methods in this class to inform us
         * of any events. Class needs to implement SecurityManagerEventHandler. */
        _ble.securityManager().setSecurityManagerEventHandler(this);

        /* privacy */

        _ble.gap().enablePrivacy(true);

        if (error) {
            printf("Error enabling privacy.\r\n");
            _event_queue.break_dispatch();
            return;
        }

        start();
    };

    /** This is called by Gap to notify the application we connected */
    virtual void on_connect(const Gap::ConnectionCallbackParams_t *connection_event)
    {
        printf("Connected to: ");
        print_address(connection_event->peerAddr);
        printf("Resolvable address: ");
        print_address(connection_event->peerResolvableAddr);

        _handle = connection_event->handle;
    };

    /** This is called by Gap to notify the application we disconnected */
    void on_disconnect(const Gap::DisconnectionCallbackParams_t *event)
    {
        if (_bonded) {
            /* we have connected to and bonded with the other device, from now
             * on we will use the second start function and stay in the same role
             * as peripheral or central */
            printf("Disconnected.\r\n");
            _event_queue.call_in(1000, this, &PrivacyDevice::start_after_bonding);
        } else {
            printf("Failed to bond.\r\n");
            _event_queue.break_dispatch();
        }
    };

    /** When scanning on advertising time runs out this is called */
    void on_timeout(const Gap::TimeoutSource_t source)
    {
        /* if we failed to find the other device, abort so that we change roles */
        printf("Haven't seen other device, switch modes.\r\n");
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

    void print_local_address()
    {
        /* show what address we are using now */
        Gap::AddressType_t addr_type;
        Gap::Address_t addr;
        _ble.gap().getAddress(&addr_type, addr);
        printf("Device address: ");
        print_address(addr);

        if (!_seeded) {
            _seeded = true;
            /* use the address as a seed */
            uint8_t* random_data = addr;
            srand(*((unsigned int*)random_data));
        }
    }

public:
    static bool _seeded;

protected:
    BLE &_ble;
    events::EventQueue &_event_queue;
    ble::connection_handle_t _handle;
    bool _bonded;
    bool _whitelist_generated;

private:
    DigitalOut _led1;
};

/** A peripheral device will advertise and accept the connections */
class PrivacyPeripheral : public PrivacyDevice {
public:
    PrivacyPeripheral(BLE &ble, events::EventQueue &event_queue)
        : PrivacyDevice(ble, event_queue) { }

    /** Set up and start advertising accepting anyone */
    virtual void start()
    {
        if (!set_advertising_data()) {
            return;
        }

        Gap::PeripheralPrivacyConfiguration_t privacy_configuration = {
            /* use_non_resolvable_random_address */ false,
            Gap::PeripheralPrivacyConfiguration_t::PERFORM_PAIRING_PROCEDURE
        };

        _ble.gap().setPeripheralPrivacyConfiguration(&privacy_configuration);
        _ble.gap().setAdvertisingPolicyMode(Gap::ADV_POLICY_IGNORE_WHITELIST);

        if (!start_advertising()) {
            return;
        }
    };

    /** advertise and filer based on known devices */
    virtual void start_after_bonding()
    {
        Gap::PeripheralPrivacyConfiguration_t privacy_configuration = {
            /* use_non_resolvable_random_address */ false,
            Gap::PeripheralPrivacyConfiguration_t::REJECT_NON_RESOLVED_ADDRESS
        };

        _ble.gap().setPeripheralPrivacyConfiguration(&privacy_configuration);

        /* enable filtering only if a whitelist has been created and set */
        if (_whitelist_generated) {
            _ble.gap().setAdvertisingPolicyMode(Gap::ADV_POLICY_FILTER_ALL_REQS);
        } else {
            /* it's illegal to apply a whitelist if there are no entries present */
            _ble.gap().setAdvertisingPolicyMode(Gap::ADV_POLICY_IGNORE_WHITELIST);
        }

        start_advertising();
    }

    /* helper functions */

private:
    bool set_advertising_data()
    {
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

        ble_error_t error = _ble.gap().setAdvertisingPayload(advertising_data);

        if (error) {
            printf("Error during Gap::setAdvertisingPayload\r\n");
            _event_queue.break_dispatch();
            return false;
        }

        return true;
    }

    bool start_advertising()
    {
        _ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
        _ble.gap().setAdvertisingInterval(20);

        if (_bonded) {
            /* if we bonded it means we have found the other device, from now on
             * wait at each step until completion */
            _ble.gap().setAdvertisingTimeout(0);
        } else {
            /* since we have two boards which might start running this example at the same time
             * we randomise the interval of advertising to have them meet when one is advertising
             * and the other one is scanning (we use their random address as source of randomness) */
            const uint16_t random_interval = 1 + rand() % 5;
            _ble.gap().setAdvertisingTimeout(random_interval);
        }

        ble_error_t error = _ble.gap().startAdvertising();

        if (error) {
            printf("Error during Gap::startAdvertising.\r\n");
            _event_queue.break_dispatch();
            return false;
        }

        printf("Advertising...\r\n");

        print_local_address();

        return true;
    }

};

/** A central device will scan and connect to a peer. */
class PrivacyCentral : public PrivacyDevice {
public:
    PrivacyCentral(BLE &ble, events::EventQueue &event_queue)
        : PrivacyDevice(ble, event_queue),
          _is_connecting(false) { }

    /** start scanning and attach a callback that will handle advertisements
     *  and scan requests responses */
    virtual void start()
    {
        Gap::CentralPrivacyConfiguration_t privacy_configuration = {
            /* use_non_resolvable_random_address */ false,
            Gap::CentralPrivacyConfiguration_t::DO_NOT_RESOLVE
        };

        _ble.gap().setCentralPrivacyConfiguration(&privacy_configuration);
        _ble.gap().setScanningPolicyMode(Gap::SCAN_POLICY_IGNORE_WHITELIST);

        start_scanning();
    }

    virtual void start_after_bonding()
    {
        Gap::CentralPrivacyConfiguration_t privacy_configuration = {
            /* use_non_resolvable_random_address */ false,
            Gap::CentralPrivacyConfiguration_t::RESOLVE_AND_FILTER
        };

        _ble.gap().setCentralPrivacyConfiguration(&privacy_configuration);

        /* enable filtering only if a whitelist has been created and set */
        if (_whitelist_generated) {
            _ble.gap().setScanningPolicyMode(Gap::SCAN_POLICY_FILTER_ALL_ADV);
        } else {
            /* it's illegal to apply a whitelist if there are no entries present */
            _ble.gap().setScanningPolicyMode(Gap::SCAN_POLICY_IGNORE_WHITELIST);
        }

        start_scanning();
    }

    /* callbacks */

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
            const uint8_t type = params->advertisingData[i + 1];
            const uint8_t *value = params->advertisingData + i + 2;

            if ((type == GapAdvertisingData::FLAGS)) {
                /* connect to discoverable devices only */
                if (!(*value & GapAdvertisingData::LE_GENERAL_DISCOVERABLE)) {
                    return;
                }
            } else if (type == GapAdvertisingData::COMPLETE_LOCAL_NAME) {
                /* connect based on the name of the device */
                if (strcmp((const char*)DEVICE_NAME, (const char*)value) == 0) {
                    _ble.gap().stopScan();

                    ble_error_t error = _ble.gap().connect(
                        params->peerAddr, params->peerAddrType,
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
            }

            i += record_length;
        }
    };

    /* TODO: remove this, pairing should be done by the strategy automatically */
    /** This is called by Gap to notify the application we connected */
    virtual void on_connect(const Gap::ConnectionCallbackParams_t *connection_event)
    {
        PrivacyDevice::on_connect(connection_event);

        _ble.securityManager().requestPairing(_handle);
    };

    /* helper functions */
private:
    bool start_scanning() {
        if (_bonded) {
            /* if we bonded it means we have found the other device, from now on
             * wait at each step until completion */
            _ble.gap().setScanTimeout(0);
        } else {
            /* otherwise only scan for a limited time before changing roles again
             * if we fail to find the other device */
            _ble.gap().setScanTimeout(5);
        }

        _is_connecting = false;

        ble_error_t error = _ble.gap().startScan(this, &PrivacyCentral::on_scan);

        if (error) {
            printf("Error during Gap::startScan %d\r\n", error);
            _event_queue.break_dispatch();
            return false;
        }

        printf("Scanning...\r\n");

        print_local_address();

        return true;
    }

private:
    bool _is_connecting;
};

/* only seed the random number generation once per application run */
bool PrivacyDevice::_seeded = false;

int main()
{
    BLE& ble = BLE::Instance();
    events::EventQueue queue;

    while(1) {
        {
            printf("\r\n * Device is a peripheral *\r\n\r\n");
            PrivacyPeripheral peripheral(ble, queue);
            peripheral.run();
        }
        {
            printf("\r\n * Device is a central *\r\n\r\n");
            PrivacyCentral peripheral(ble, queue);
            peripheral.run();
        }
    }

    return 0;
}
