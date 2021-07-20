/*
 * Mbed-OS Microcontroller Library
 * Copyright (c) 2020-2021 Embedded Planet
 * Copyright (c) 2020-2021 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
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
 * limitations under the License
 */

#include "PeriodicBlockDeviceEraser.h"

PeriodicBlockDeviceEraser::PeriodicBlockDeviceEraser(mbed::BlockDevice &bd,
                                                     events::EventQueue &queue) : _bd(bd), _queue(queue) {
}

PeriodicBlockDeviceEraser::~PeriodicBlockDeviceEraser() {
    _queue.cancel(_erase_event_id);
}

int PeriodicBlockDeviceEraser::start_erase(bd_addr_t addr, bd_size_t size,
                                           bd_size_t erase_size, PeriodicBlockDeviceCallback_t cb) {

    /* Make sure the total size is a multiple of erase_size */
    if((size % erase_size) != 0) {
        return 1;
    }

    /* Make sure the erase size is a multiple of the BD erase size */
    if((erase_size % _bd.get_erase_size()) != 0) {
        return 1;
    }

    _done = false;
    _bd_error = mbed::BD_ERROR_OK;
    _addr = addr;
    _end_addr = addr + size;
    _erase_size = erase_size;
    _cb = cb;

    /* Start the periodic erase event calls */
    _erase_event_id = _queue.call(mbed::callback(this, &PeriodicBlockDeviceEraser::erase));

    return 0;
}

void PeriodicBlockDeviceEraser::erase() {
    _bd_error = _bd.erase(_addr, _erase_size);

    /* If there was an error in erasing, stop now and report to the application */
    if(_bd_error) {
        if(_cb) {
            _cb(_bd_error);
        }
        _done = true;
        return;
    }

    _addr += _erase_size;
    if(_addr < _end_addr) {
        _erase_event_id = _queue.call(mbed::callback(this, &PeriodicBlockDeviceEraser::erase));
    } else {
        if(_cb) {
            _cb(_bd_error);
        }
        _done = true;
    }
}