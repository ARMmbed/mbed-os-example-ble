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

#include <mbed-events/events.h>
#include <mbed.h>
#include "ble/BLE.h"
#include "ble/Gap.h"
#include "HeartRateSecService.h"
#include "ble/services/DeviceInformationService.h"

DigitalOut led1(LED1);

const static char     DEVICE_NAME[]        = "HRM_SEC";
static const uint16_t uuid16_list[]        = {GattService::UUID_HEART_RATE_SERVICE,
                                              GattService::UUID_DEVICE_INFORMATION_SERVICE};
static volatile bool  triggerSensorPolling = false;

static uint8_t hrmCounter = 100; // init HRM to 100bps
static HeartRateSecService * hrService;



static EventQueue eventQueue(
    /* event count */ 16 * /* event size */ 32
);

void disconnectionCallback(const Gap::DisconnectionCallbackParams_t *params)
{
    printf("Disconnected!\r\n");
    BLE::Instance().gap().startAdvertising(); // restart advertising
}

void periodicCallback(void)
{
    led1 = !led1; /* Do blinky on LED1 while we're waiting for BLE events */

    /* Note that the periodicCallback() executes in interrupt context, so it is safer to do
     * heavy-weight sensor polling from the main thread. */
    triggerSensorPolling = true;
}

void connectionCallback(const Gap::ConnectionCallbackParams_t *params)
{
    printf("Connected!\r\n");
}

void passkeyDisplayCallback(Gap::Handle_t handle, const SecurityManager::Passkey_t passkey)
{
    printf("Input passKey: ");
    for (unsigned i = 0; i < Gap::ADDR_LEN; i++) {
        printf("%c ", passkey[i]);
    }
    printf("\r\n");
}

void securitySetupCompletedCallback(Gap::Handle_t handle, SecurityManager::SecurityCompletionStatus_t status)
{
    if (status == SecurityManager::SEC_STATUS_SUCCESS) {
        printf("Security success\r\n");
    } else {
        printf("Security failed\r\n");
    }
}

void securityContexStoredCallback(Gap::Handle_t handle)
{
	printf("Peer data updated\r\n");
}

void scheduleBleEventsProcessing(BLE::OnEventsToProcessCallbackContext* context) {
    BLE &ble = BLE::Instance();
    eventQueue.post(Callback<void()>(&ble, &BLE::processEvents));
}

void onBleInitError(BLE &ble, ble_error_t error)
{
    (void)ble;
    (void)error;
   /* Initialization error handling should go here */
}

void bleInitComplete(BLE::InitializationCompleteCallbackContext *params)
{
    BLE&        ble   = params->ble;
    ble_error_t error = params->error;

    if (error != BLE_ERROR_NONE) {
        onBleInitError(ble, error);
        return;
    }
    
    /* Initialize BLE security */
    bool enableBonding = true;
    bool requireMITM   = true;
    ble.securityManager().init(enableBonding, requireMITM, SecurityManager::IO_CAPS_DISPLAY_ONLY);
    
    /* Set callback functions */
    ble.gap().onConnection(connectionCallback);
    ble.gap().onDisconnection(disconnectionCallback);
    ble.securityManager().onPasskeyDisplay(passkeyDisplayCallback);
    ble.securityManager().onSecuritySetupCompleted(securitySetupCompletedCallback);
    ble.securityManager().onSecurityContextStored(securityContexStoredCallback);
    
    /* Setup primary service. */
    hrService = new HeartRateSecService(ble, hrmCounter, HeartRateSecService::LOCATION_FINGER);

    /* Setup auxiliary service. */
    DeviceInformationService deviceInfo(ble, "ARM", "Model1", "SN1", "hw-rev1", "fw-rev1", "soft-rev1");

    /* Setup advertising. */
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_16BIT_SERVICE_IDS, (uint8_t *)uuid16_list, sizeof(uuid16_list));
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::GENERIC_HEART_RATE_SENSOR);
    ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME, (uint8_t *)DEVICE_NAME, sizeof(DEVICE_NAME));
    ble.gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);
    ble.gap().setAdvertisingInterval(1000);
    
    ble.gap().startAdvertising();
}

int main(void)
{
    led1 = 1;
    Ticker ticker;
    ticker.attach(periodicCallback, 1); // blink LED every second

    BLE &ble = BLE::Instance();
    ble.onEventsToProcess(scheduleBleEventsProcessing);
    /* Initialize BLE module */
    ble.init(bleInitComplete);

    // infinite loop
    while (1) {
        // check for trigger from periodicCallback()
        if (triggerSensorPolling && ble.getGapState().connected) {
            triggerSensorPolling = false;

            // Do blocking calls or whatever is necessary for sensor polling.
            // In our case, we simply update the HRM measurement. 
            hrmCounter++;
            
            //  100 <= HRM bps <=175
            if (hrmCounter == 175) {
                hrmCounter = 100;
            }
            
            // update bps
            hrService->updateHeartRate(hrmCounter);
        } else {
            ble.waitForEvent(); // low power wait for event
        }
        eventQueue.dispatch();
    }
}
