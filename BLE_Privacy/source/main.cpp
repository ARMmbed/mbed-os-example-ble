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
#include <algorithm>
#include "demo.h"
#include "ble/gap/AdvertisingDataParser.h"

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
 *  to store the IRK that resolve the peer. Subsequent connections will
 *  turn on filtering based on stored IRKs.
 */

static const char DEVICE_NAME[] = "Privacy";

/* we have to specify the disconnect call because of ambiguous overloads */
typedef ble_error_t (Gap::*disconnect_call_t)(ble::connection_handle_t, ble::local_disconnection_reason_t);
const static disconnect_call_t disconnect_call = &Gap::disconnect;

/** Base class for both peripheral and central. The same class that provides
 *  the logic for the application also implements the SecurityManagerEventHandler
 *  which is the interface used by the Security Manager to communicate events
 *  back to the applications. You can provide overrides for a selection of events
 *  your application is interested in.
 */
class PrivacyDevice : private mbed::NonCopyable<PrivacyDevice>,
                      public SecurityManager::EventHandler,
                      public ble::Gap::EventHandler
{
public:
    PrivacyDevice(BLE &ble, events::EventQueue &event_queue) :
        _ble(ble),
        _event_queue(event_queue),
        _handle(0),
        _bonded(false),
        _led1(LED1, 0) { };

    virtual ~PrivacyDevice() {
        _ble.onEventsToProcess(NULL);
    };

    /** Start BLE interface initialisation */
    void run()
    {
        /* to show we're running we'll blink every 500ms */
        _event_queue.call_every(500, this, &PrivacyDevice::blink);

        /* this will inform us off all events so we can schedule their handling
         * using our event queue */
        _ble.onEventsToProcess(
            makeFunctionPointer(this, &PrivacyDevice::schedule_ble_events)
        );

        /* handle gap events */
        _ble.gap().setEventHandler(this);

        if (_ble.hasInitialized()) {
            /* ble instance already initialised, skip init and start activity */
            start();
        } else {
            ble_error_t error = _ble.init(this, &PrivacyDevice::on_init_complete);

            if (error) {
                printf("Error returned by BLE::init.\r\n");
                return;
            }
        }

        /* this will not return until shutdown */
        _event_queue.dispatch_forever();
    };

    /** Override to start chosen activity when initialisation completes */
    virtual void start() = 0;

    /** Override to start chosen activity after initial bonding */
    virtual void start_after_bonding() = 0;

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

        /* gap events also handled by this class */
        _ble.gap().setEventHandler(this);

        /* privacy */

        error = _ble.gap().enablePrivacy(true);

        if (error) {
            printf("Error enabling privacy.\r\n");
            _event_queue.break_dispatch();
            return;
        }

        start();
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

private:
    /* Event handler */

    /** Inform the application of pairing */
    virtual void pairingResult(
        ble::connection_handle_t connectionHandle,
        SecurityManager::SecurityCompletionStatus_t result
    ) {
        if (result == SecurityManager::SEC_STATUS_SUCCESS) {
            printf("Pairing successful\r\n");
            _bonded = true;
        } else {
            printf("Pairing failed\r\n");
        }

        /* disconnect in 2s */
        _event_queue.call_in(
            2000,
            &_ble.gap(),
            disconnect_call,
            connectionHandle,
            ble::local_disconnection_reason_t(ble::local_disconnection_reason_t::USER_TERMINATION)
        );
    }

    /** This is called by Gap to notify the application we connected */
    virtual void onConnectionComplete(const ble::ConnectionCompleteEvent &event)
    {
        printf("Connected to peer: ");
        print_address(event.getPeerAddress().data());
        printf("Peer random resolvable address: ");
        print_address(event.getPeerResolvablePrivateAddress().data());

        _handle = event.getConnectionHandle();

        if (_bonded) {
            /* disconnect in 2s */
            _event_queue.call_in(
                2000,
                &_ble.gap(),
                disconnect_call,
                _handle,
                ble::local_disconnection_reason_t(ble::local_disconnection_reason_t::USER_TERMINATION)
            );
        }
    };

    /** This is called by Gap to notify the application we disconnected */
    virtual void onDisconnectionComplete(const ble::DisconnectionEvent &event)
    {
        if (_bonded) {
            /* we have connected to and bonded with the other device, from now
             * on we will use the second start function and stay in the same role
             * as peripheral or central */
            printf("Disconnected.\r\n");
            _event_queue.call_in(2000, this, &PrivacyDevice::start_after_bonding);
        } else {
            printf("Failed to bond.\r\n");
            _event_queue.break_dispatch();
        }
    };

    virtual void onScanTimeout(const ble::ScanTimeoutEvent &)
    {
        /* if we failed to find the other device, abort so that we change roles */
        printf("Haven't seen other device, switch modes.\r\n");
        _event_queue.break_dispatch();
    }

public:
    static bool _seeded;

protected:
    BLE &_ble;
    events::EventQueue &_event_queue;
    ble::connection_handle_t _handle;
    bool _bonded;

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

        start_advertising();
    };

    /** advertise and filter based on known devices */
    virtual void start_after_bonding()
    {
        Gap::PeripheralPrivacyConfiguration_t privacy_configuration = {
            /* use_non_resolvable_random_address */ false,
            Gap::PeripheralPrivacyConfiguration_t::REJECT_NON_RESOLVED_ADDRESS
        };

        _ble.gap().setPeripheralPrivacyConfiguration(&privacy_configuration);

        start_advertising();
    }

    /* helper functions */

private:
    bool set_advertising_data()
    {
        uint8_t adv_buffer[ble::LEGACY_ADVERTISING_MAX_SIZE];
        /* use the helper to build the payload */
        ble::AdvertisingDataBuilder adv_data_builder(
            adv_buffer
        );

        adv_data_builder.setFlags();
        adv_data_builder.setName(DEVICE_NAME);

        /* Set payload for the set */
        ble_error_t error = _ble.gap().setAdvertisingPayload(
            ble::LEGACY_ADVERTISING_HANDLE,
            adv_data_builder.getAdvertisingData()
        );

        if (error) {
            print_error(error, "Gap::setAdvertisingPayload() failed");
            _event_queue.break_dispatch();
            return false;
        }

        return true;
    }

    bool start_advertising()
    {
        ble::AdvertisingParameters adv_parameters(
            ble::advertising_type_t::CONNECTABLE_UNDIRECTED
        );

        ble_error_t error = _ble.gap().setAdvertisingParameters(
            ble::LEGACY_ADVERTISING_HANDLE,
            adv_parameters
        );

        if (error) {
            print_error(error, "Gap::setAdvertisingParameters() failed");
            return false;
        }

        if (_bonded) {
            /* if we bonded it means we have found the other device, from now on
             * wait at each step until completion */
            error = _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);
        } else {
            /* since we have two boards which might start running this example at the same time
             * we randomise the interval of advertising to have them meet when one is advertising
             * and the other one is scanning (we use their random address as source of randomness) */
            ble::millisecond_t random_duration_ms((1 + rand() % 5) * 1000);
            ble::adv_duration_t random_duration(random_duration_ms);
            error = _ble.gap().startAdvertising(
                ble::LEGACY_ADVERTISING_HANDLE,
                random_duration
            );
        }

        if (error) {
            print_error(error, "Gap::startAdvertising() failed");
            _event_queue.break_dispatch();
            return false;
        }

        printf("Advertising...\r\n");

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

        start_scanning();
    }

    virtual void start_after_bonding()
    {
        Gap::CentralPrivacyConfiguration_t privacy_configuration = {
            /* use_non_resolvable_random_address */ false,
            Gap::CentralPrivacyConfiguration_t::RESOLVE_AND_FILTER
        };

        _ble.gap().setCentralPrivacyConfiguration(&privacy_configuration);

        start_scanning();
    }

    /* helper functions */
private:
    bool start_scanning() {
        ble_error_t error;
        ble::ScanParameters scan_params;
        _ble.gap().setScanParameters(scan_params);

        _is_connecting = false;

        if (_bonded) {
            /* if we bonded it means we have found the other device, from now on
             * wait at each step until completion */
            error = _ble.gap().startScan(
                ble::duplicates_filter_t::DISABLE,
                ble::scan_duration_t(0)
            );
        } else {
            /* otherwise only scan for a limited time before changing roles again
             * if we fail to find the other device */
            error = _ble.gap().startScan(
                ble::duplicates_filter_t::DISABLE,
                ble::scan_duration_t(ble::millisecond_t(4000))
            );
        }

        if (error) {
            printf("Error during Gap::startScan %d\r\n", error);
            _event_queue.break_dispatch();
            return false;
        }

        printf("Scanning...\r\n");

        return true;
    }

private:
    /* Event handler */

    /** Look at scan payload to find a peer device and connect to it */
    virtual void onAdvertisingReport(const ble::AdvertisingReportEvent &event)
    {
        /* don't bother with analysing scan result if we're already connecting */
        if (_is_connecting) {
            return;
        }

        ble::AdvertisingDataParser adv_data(event.getAdvertisingData());

        /* parse the advertising payload, looking for a discoverable device */
        while (adv_data.hasNext()) {
            ble::AdvertisingDataParser::element_t field = adv_data.next();

            /* connect to a known device by name */
            if (field.type == ble::adv_data_type_t::COMPLETE_LOCAL_NAME &&
                field.value.size() == sizeof(DEVICE_NAME) &&
                (memcmp(field.value.data(), DEVICE_NAME, sizeof(DEVICE_NAME)) == 0)) {

                printf("We found a connectable device\r\n");

                ble_error_t error = _ble.gap().stopScan();

                if (error) {
                    print_error(error, "Error caused by Gap::stopScan");
                    return;
                }

                const ble::ConnectionParameters connection_params;

                error = _ble.gap().connect(
                    event.getPeerAddressType(),
                    event.getPeerAddress(),
                    connection_params
                );

                if (error) {
                    print_error(error, "Error caused by Gap::connect");
                    return;
                }

                /* we may have already scan events waiting
                 * to be processed so we need to remember
                 * that we are already connecting and ignore them */
                _is_connecting = true;

                return;

            }
        }
    }

private:
    bool _is_connecting;
};

/* only seed the random number generation once per application run */
bool PrivacyDevice::_seeded = false;

int main()
{
    BLE& ble = BLE::Instance();

    while(1) {
        {
            events::EventQueue queue;
            printf("\r\n * Device is a peripheral *\r\n\r\n");
            PrivacyPeripheral peripheral(ble, queue);
            peripheral.run();
        }
        {
            events::EventQueue queue;
            printf("\r\n * Device is a central *\r\n\r\n");
            PrivacyCentral central(ble, queue);
            central.run();
        }
    }

    return 0;
}
