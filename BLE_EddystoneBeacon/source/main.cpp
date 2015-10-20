/* mbed Microcontroller Library
 * Copyright (c) 2006-2015 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mbed.h"
#include "ble/BLE.h"
#include "ble/services/EddystoneService.h"

DigitalOut led1(LED1, 1);

static uint8_t UIDnamespace[] = {0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xAA}; // 10Bytes for Namespace UUID
static uint8_t UIDinstance[]  = {0xbb,0xcc,0xdd,0xee,0xff,0x00}; // 6Bytes for Instance UUID
static char Url[] = "http://www.mbed.org";
static int8_t radioTxPower = 20;
static int8_t advTxPower = -20;
static uint16_t beaconPeriodus = 1000;
static uint8_t tlmVersion = 0x00;

static int battery = 0;
static int temp = 0;

EddystoneService *eddyBeaconPtr;

void blinkCallback(void)
{
    led1 = !led1; /* Do blinky on LED1 while we're waiting for BLE events */
}

/* Optional Function to update Eddystone beacon TLM frame battery voltage */
void tlmBatteryCallback(void){
    eddyBeaconPtr->updateTlmBatteryVoltage(battery++);
}

/* Optional Function to update Eddystone beacon TLM frame temperature */
void tlmTemperatureCallback(void){
    eddyBeaconPtr->updateTlmBeaconTemp(temp++);
}

void app_start(int, char**)
{
    minar::Scheduler::postCallback(blinkCallback).period(minar::milliseconds(500));

    BLE &ble = BLE::Instance();

    /* Setup Eddystone Service */
    eddyBeaconPtr = new EddystoneService(ble, beaconPeriodus, radioTxPower);

    ble.init();

    /* Set Eddystone Frame Data (TLM,UID,URI...etc) */
    eddyBeaconPtr->setTLMFrameData(tlmVersion,5.0);
    eddyBeaconPtr->setURLFrameData(advTxPower, Url, 2.0);
    eddyBeaconPtr->setUIDFrameData(advTxPower, UIDnamespace, UIDinstance, 3.0);

    /* Callbacks for temperature / battery updates */
    minar::Scheduler::postCallback(tlmTemperatureCallback).period(minar::milliseconds(2000));
    minar::Scheduler::postCallback(tlmBatteryCallback).period(minar::milliseconds(1000));

    /* Start Advertising the eddystone service. */
    eddyBeaconPtr->start();
    ble.gap().startAdvertising();
}
