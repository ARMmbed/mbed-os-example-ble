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

#include "BlockDeviceFOTAEventHandler.h"

#include "BlockDevice.h"
#include "mbed-trace/mbed_trace.h"

#define TRACE_GROUP "FOTA"

BlockDeviceFOTAEventHandler::BlockDeviceFOTAEventHandler(mbed::BlockDevice &bd, events::EventQueue &eq) :
                                                         _bd(bd), _eq(eq)
{
}

BlockDeviceFOTAEventHandler::~BlockDeviceFOTAEventHandler()
{
    delete _bd_eraser;
}

FOTAService::StatusCode_t BlockDeviceFOTAEventHandler::on_binary_stream_written(
        FOTAService &fota_service,
        mbed::Span<const uint8_t> buffer)
{
    tr_info("BSC written: programming %s at address %" PRIu64 "",
            mbed_trace_array(buffer.data(), buffer.size()),
            _addr);

    int error = _bd.program(buffer.data(), _addr, buffer.size());
    if (error) {
        tr_error("Programming block device failed: 0x%x", error);
        return FOTAService::FOTA_STATUS_MEMORY_ERROR;
    }

    _addr += buffer.size();

    return FOTAService::FOTA_STATUS_OK;
}

GattAuthCallbackReply_t BlockDeviceFOTAEventHandler::on_control_written(
        FOTAService &fota_service,
        mbed::Span<const uint8_t> buffer)
{
    _fota_service = &fota_service;

    switch (buffer[0]) {
        case FOTAService::FOTA_NO_OP:
        {
            break;
        }

        case FOTAService::FOTA_START:
        {
            tr_info("Starting FOTA session");
            fota_service.start_fota_session();

            /* We must erase the update block device before accepting BSC writes
             * Therefore, set the status characteristic to XOFF to initiate a "delayed start"
             */
            fota_service.set_xoff();

            /* Delete previously allocated eraser */
            delete _bd_eraser;

            /* Erase update block device */
            _bd_eraser = new PeriodicBlockDeviceEraser(_bd, _eq);
            tr_info("Erasing block device: size=%" PRIu64 "",
                    static_cast<uint64_t>(_bd.size()));
            _bd_eraser->start_erase(0, _bd.size(),
                    mbed::callback(this, &BlockDeviceFOTAEventHandler::on_bd_erased));

            break;
        }

        case FOTAService::FOTA_STOP:
        {
            tr_info("Stopping FOTA session");
            fota_service.stop_fota_session();
            break;
        }

        case FOTAService::FOTA_COMMIT:
        {
            /* Application-specific */
            break;
        }

        default:
        {
            return static_cast<GattAuthCallbackReply_t>
            (FOTAService::AUTH_CALLBACK_REPLY_ATTERR_UNSUPPORTED_OPCODE);
        }
    }

    return AUTH_CALLBACK_REPLY_SUCCESS;
}

void BlockDeviceFOTAEventHandler::on_bd_erased(int result)
{
    if (result != mbed::BD_ERROR_OK) {
        tr_error("Failed to erase block device: notifying client");
        _fota_service->notify_status(FOTAService::FOTA_STATUS_MEMORY_ERROR);
    } else {
        tr_info("Successfully erased the update block Device");
        _fota_service->set_xon();
        _fota_service->notify_status(FOTAService::FOTA_STATUS_OK);
    }
}
