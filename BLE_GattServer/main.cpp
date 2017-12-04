#include <stdio.h>

#include "platform/Callback.h"
#include "events/Eventqueue.h"
#include "platform/NonCopyable.h"

#include "ble/BLE.h"
#include "ble/Gap.h"
#include "ble/GattClient.h"
#include "ble/GapAdvertisingParams.h"
#include "ble/GapAdvertisingData.h"
#include "ble/GattServer.h"

using mbed::callback;

/**
 * A Clock service that demonstrate the GattServer features.
 *
 * The clock service host three characteristics that respectively model the
 * current hour, minute and second. The value of the second characteristic is
 * incremented automatically by the system.
 *
 * When the value of the second characteristic reach 60, it is reset to 0 and
 * the value of the minute characteristic is incremented.
 *
 * Similarly when value of the minute characteristic reach 60, it is reset and
 * the value of the hour characteristic is incremented.
 *
 * Finaly when the value of the hour characteristic reach 24, it is reset to 0.
 *
 * A client can subscribe to updates of the clock characteristics and get
 * notified when one of the value is changed. Clients can also change value of
 * the second, minute and hour characteristric.
 */
class ClockService {
    typedef ClockService Self;

public:
    ClockService() :
        _hour_char("485f4145-52b9-4644-af1f-7a6b9322490f", 0),
        _minute_char("0a924ca7-87cd-4699-a3bd-abdcd9cf126a", 0),
        _second_char("8dd6a1b7-bc75-4741-8a26-264af75807de", 0),
        _clock_service(
            /* uuid */ "51311102-030e-485f-b122-f8f381aa84ed",
            /* characteristics */ _clock_characteristics,
            /* numCharacteristics */ sizeof(_clock_characteristics) /
                                     sizeof(_clock_characteristics[0])
        ),
        _server(NULL),
        _event_queue(NULL)
    {
        // update internal pointers (value, descriptors and characteristics array)
        _clock_characteristics[0] = &_hour_char;
        _clock_characteristics[1] = &_minute_char;
        _clock_characteristics[2] = &_second_char;

        // setup authorization handlers
        _hour_char.setWriteAuthorizationCallback(this, &Self::authorize_client_write);
        _minute_char.setWriteAuthorizationCallback(this, &Self::authorize_client_write);
        _second_char.setWriteAuthorizationCallback(this, &Self::authorize_client_write);
    }

    void start(GattServer &server, events::EventQueue &event_queue)
    {
        if (_event_queue) {
            return;
        }

        _server = &server;
        _event_queue = &event_queue;

        // register the service
        ble_error_t err = _server->addService(_clock_service);

        if (err) {
            printf("Error %u during demo service registration.\r\n", err);
            return;
        }

        // read write handler
        _server->onDataSent(as_cb(&Self::when_data_sent));
        _server->onDataWritten(as_cb(&Self::when_data_written));
        _server->onDataRead(as_cb(&Self::when_data_read));

        // updates subscribtion handlers
        _server->onUpdatesEnabled(as_cb(&Self::when_update_enabled));
        _server->onUpdatesDisabled(as_cb(&Self::when_update_disabled));
        _server->onConfirmationReceived(as_cb(&Self::when_confirmation_received));

        // print the handles
        printf("clock service registered\r\n");
        printf("service handle: %u\r\n", _clock_service.getHandle());
        printf("\thour characteristic value handle %u\r\n", _hour_char.getValueHandle());
        printf("\tminute characteristic value handle %u\r\n", _minute_char.getValueHandle());
        printf("\tsecond characteristic value handle %u\r\n", _second_char.getValueHandle());

        _event_queue->call_every(1000 /* ms */, callback(this, &Self::increment_second));
    }

private:

//
// Handlers called after a characteristic has been read, writen or notified
//
    void when_data_sent(unsigned count)
    {
        printf("sent %u updates\r\n", count);
    }

    void when_data_written(const GattWriteCallbackParams *e)
    {
        printf("data written:\r\n");
        printf("\tconnection handle: %u\r\n", e->connHandle);
        printf("\tattribute handle: %u", e->handle);
        if (e->handle == _hour_char.getValueHandle()) {
            printf(" (hour characteristic)\r\n");
        } else if (e->handle == _minute_char.getValueHandle()) {
            printf(" (minute characteristic)\r\n");
        } else if (e->handle == _second_char.getValueHandle()) {
            printf(" (second characteristic)\r\n");
        } else {
            printf("\r\n");
        }
        printf("\twrite operation: %u\r\n", e->writeOp);
        printf("\toffset: %u\r\n", e->offset);
        printf("\tlength: %u\r\n", e->len);
        printf("\t data: ");

        for (size_t i = 0; i < e->len; ++i) {
            printf("%02X", e->data[i]);
        }

        printf("\r\n");
    }

    void when_data_read(const GattReadCallbackParams *e)
    {
        printf("data read:\r\n");
        printf("\tconnection handle: %u\r\n", e->connHandle);
        printf("\tattribute handle: %u", e->handle);
        if (e->handle == _hour_char.getValueHandle()) {
            printf(" (hour characteristic)\r\n");
        } else if (e->handle == _minute_char.getValueHandle()) {
            printf(" (minute characteristic)\r\n");
        } else if (e->handle == _second_char.getValueHandle()) {
            printf(" (second characteristic)\r\n");
        } else {
            printf("\r\n");
        }
    }

//
// Client Characteristic Configuration Descriptors (CCCD) events handlers
//
    void when_update_enabled(GattAttribute::Handle_t handle)
    {
        printf("update enabled on handle %d\r\n", handle);
    }

    void when_update_disabled(GattAttribute::Handle_t handle)
    {
        printf("update disabled on handle %d\r\n", handle);
    }

    void when_confirmation_received(GattAttribute::Handle_t handle)
    {
        printf("confirmation received on handle %d\r\n", handle);
    }

//
// Write Authorization handlers for the second, minute and hour characteristics
//
    void authorize_client_write(GattWriteAuthCallbackParams *e)
    {
        printf("characteristic %u write authorization\r\n", e->handle);

        if (e->offset != 0) {
            printf("Error invalid offset\r\n");
            e->authorizationReply = AUTH_CALLBACK_REPLY_ATTERR_INVALID_OFFSET;
            return;
        }

        if (e->len != 1) {
            printf("Error invalid len\r\n");
            e->authorizationReply = AUTH_CALLBACK_REPLY_ATTERR_INVALID_ATT_VAL_LENGTH;
            return;
        }

        if ((e->data[0] >= 60) ||
            ((e->data[0] >= 24) && (e->handle == _hour_char.getValueHandle()))) {
            printf("Error invalid data\r\n");
            e->authorizationReply = AUTH_CALLBACK_REPLY_ATTERR_WRITE_NOT_PERMITTED;
            return;
        }

        e->authorizationReply = AUTH_CALLBACK_REPLY_SUCCESS;
    }

//
// Incrementation of seconds, minutes and hours characteristics
//
    void increment_second(void)
    {
        uint8_t second = 0;
        ble_error_t err = _second_char.get(*_server, second);
        if (err) {
            printf("read of the second value returned error %u\r\n", err);
            return;
        }

        second = (second + 1) % 60;

        err = _second_char.set(*_server, second);
        if (err) {
            printf("write of the second value returned error %u\r\n", err);
            return;
        }

        if (second == 0) {
            increment_minute();
        }
    }

    void increment_minute(void)
    {
        uint8_t minute = 0;
        ble_error_t err = _minute_char.get(*_server, minute);
        if (err) {
            printf("read of the minute value returned error %u\r\n", err);
            return;
        }

        minute = (minute + 1) % 60;

        err = _minute_char.set(*_server, minute);
        if (err) {
            printf("write of the minute value returned error %u\r\n", err);
            return;
        }

        if (minute == 0) {
            increment_hour();
        }
    }

    void increment_hour(void)
    {
        uint8_t hour = 0;
        ble_error_t err = _hour_char.get(*_server, hour);
        if (err) {
            printf("read of the hour value returned error %u\r\n", err);
            return;
        }

        hour = (hour + 1) % 24;

        err = _hour_char.set(*_server, hour);
        if (err) {
            printf("write of the hour value returned error %u\r\n", err);
            return;
        }
    }

private:
    /**
     * Helper that construct an event handler from a member function of this
     * instance.
     */
    template<typename Arg>
    FunctionPointerWithContext<Arg> as_cb(void (Self::*member)(Arg))
    {
        return makeFunctionPointer(this, member);
    }

    /**
     * Read, Write, Notify, Indicate  Characteristic declaration helper.
     *
     * @tparam T type of data held by the characteristic.
     */
    template<typename T>
    class ReadWriteNotifyIndicateCharacteristic : public GattCharacteristic {
    public:
        /**
         * Construct a characteristic that can be read or written and emit
         * notification or indication.
         *
         * @param[in] uuid The UUID of the characteristic.
         * @param[in] initial_value Initial value contained by the characteristic.
         */
        ReadWriteNotifyIndicateCharacteristic(const UUID & uuid, const T& initial_value) :
            GattCharacteristic(
                /* UUID */ uuid,
                /* Initial value */ &_value,
                /* Value size */ sizeof(_value),
                /* Value capacity */ sizeof(_value),
                /* Properties */ GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_READ |
                                GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_WRITE |
                                GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY |
                                GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_INDICATE,
                /* Descriptors */ NULL,
                /* Num descriptors */ 0,
                /* variable len */ false
            ),
            _value(initial_value) {
        }

        /**
         * Get the value of this characteristic.
         *
         * @param[in] server GattServer instance that contain the characteristic
         * value.
         * @param[in] dst Variable that will receive the characteristic value.
         *
         * @return BLE_ERROR_NONE in case of success or an appropriate error code.
         */
        ble_error_t get(GattServer &server, T& dst) const
        {
            uint16_t value_length = sizeof(dst);
            return server.read(getValueHandle(), &dst, &value_length);
        }

        /**
         * Assign a new value to this characteristic.
         *
         * @param[in] server GattServer instance that will receive the new value.
         * @param[in] value The new value to set.
         * @param[in] local_only Flag that determine if the change should be kept
         * locally or forwarded to subscribed clients.
         */
        ble_error_t set(
            GattServer &server, const uint8_t &value, bool local_only = false
        ) const {
            return server.write(getValueHandle(), &value, sizeof(value), local_only);
        }

    private:
        uint8_t _value;
    };

    ReadWriteNotifyIndicateCharacteristic<uint8_t> _hour_char;
    ReadWriteNotifyIndicateCharacteristic<uint8_t> _minute_char;
    ReadWriteNotifyIndicateCharacteristic<uint8_t> _second_char;

    // list of the characteristics of the clock service
    GattCharacteristic* _clock_characteristics[3];

    // demo service
    GattService _clock_service;

    GattServer* _server;
    events::EventQueue *_event_queue;
};


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
    BLEProcess(events::EventQueue &event_queue, BLE &ble_interface, ClockService& service) :
        _event_queue(event_queue),
        _ble_interface(ble_interface),
        _demonstration_service(service) {
    }

    ~BLEProcess()
    {
        stop();
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

        printf("Adding demo service\r\n");
        _demonstration_service.start(_ble_interface.gattServer(), _event_queue);

        Gap &gap = _ble_interface.gap();
        ble_error_t error = gap.setAdvertisingPayload(make_advertising_data());
        if (error) {
            printf("Error %u during gap.setAdvertisingPayload\r\n", error);
            return;
        }

        gap.setAdvertisingParams(make_advertising_params());

        gap.onConnection(this, &BLEProcess::when_connection);
        gap.onDisconnection(this, &BLEProcess::when_disconnection);

        start_advertising();
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

    void start_advertising(void)
    {
        ble_error_t error = _ble_interface.gap().startAdvertising();
        if (error) {
            printf("Error %u during gap.startAdvertising.\r\n", error);
            return;
        } else {
            printf("Advertising started.\r\n");
        }
    }

    static GapAdvertisingData make_advertising_data(void)
    {
        static const uint8_t device_name[] = "GattServer";
        GapAdvertisingData advertising_data;

        // add advertising flags
        advertising_data.addFlags(
            GapAdvertisingData::LE_GENERAL_DISCOVERABLE |
            GapAdvertisingData::BREDR_NOT_SUPPORTED
        );

        // add device name
        advertising_data.addData(
            GapAdvertisingData::COMPLETE_LOCAL_NAME,
            device_name,
            sizeof(device_name)
        );

        return advertising_data;
    }

    static GapAdvertisingParams make_advertising_params(void)
    {
        return GapAdvertisingParams(
            /* type */ GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED,
            /* interval */ GapAdvertisingParams::MSEC_TO_ADVERTISEMENT_DURATION_UNITS(500),
            /* timeout */ 0
        );
    }

    events::EventQueue &_event_queue;
    BLE &_ble_interface;
    ClockService &_demonstration_service;
};


int main() {

    BLE &ble_interface = BLE::Instance();
    events::EventQueue event_queue;
    ClockService demo_service;
    BLEProcess ble_process(event_queue, ble_interface, demo_service);

    // bind the event queue to the ble interface, initialize the interface
    // and start advertising
    ble_process.start();

    // Process the event queue.
    event_queue.dispatch_forever();

    return 0;
}
