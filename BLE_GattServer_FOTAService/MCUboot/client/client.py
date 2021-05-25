# Copyright (c) 2020-2021 Embedded Planet
# Copyright (c) 2009-2020 Arm Limited
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License

import hashlib
import logging
import asyncio

from bleak          import BleakScanner, BleakClient, BleakError
from bleak.uuids    import uuid16_dict
from typing         import Optional, Union
from logging.config import fileConfig

uuid16_dict = {v: k for k, v in uuid16_dict.items()}

fileConfig('logging.conf')

logger = logging.getLogger('logger')

# FOTA UUIDS
UUID_FOTA_SERVICE                  = "53880000-65fd-4651-ba8e-91527f06c887"
UUID_BINARY_STREAM_CHAR            = "53880001-65fd-4651-ba8e-91527f06c887"
UUID_CONTROL_CHAR                  = "53880002-65fd-4651-ba8e-91527f06c887"
UUID_STATUS_CHAR                   = "53880003-65fd-4651-ba8e-91527f06c887"
UUID_VERSION_CHAR                  = "53880004-65fd-4651-ba8e-91527f06c887"
UUID_FIRMWARE_REVISION_STRING_CHAR = "0000{0:x}-0000-1000-8000-00805f9b34fb".format(
    uuid16_dict.get("Firmware Revision String")
)

# Additional UUIDs
UUID_DEVICE_INFORMATION_SERVICE    = "0000{0:x}-0000-1000-8000-00805f9b34fb".format(
    uuid16_dict.get("Device Information")
)
UUID_CUDD                          = "0000{0:x}-0000-1000-8000-00805f9b34fb".format(
    uuid16_dict.get("Characteristic User Description")
)

# FOTA status codes
FOTA_STATUS_OK                     = bytearray(b'\x00')
FOTA_STATUS_UPDATE_SUCCESSFUL      = bytearray(b'\x01')
FOTA_STATUS_XOFF                   = bytearray(b'\x02')
FOTA_STATUS_XON                    = bytearray(b'\x03')
FOTA_STATUS_SYNC_LOST              = bytearray(b'\x04')
FOTA_STATUS_UNSPECIFIED_ERROR      = bytearray(b'\x05')
FOTA_STATUS_VALIDATION_FAILURE     = bytearray(b'\x06')
FOTA_STATUS_INSTALLATION_FAILURE   = bytearray(b'\x07')
FOTA_STATUS_OUT_OF_MEMORY          = bytearray(b'\x08')
FOTA_STATUS_MEMORY_ERROR           = bytearray(b'\x09')
FOTA_STATUS_HARDWARE_ERROR         = bytearray(b'\x0a')
FOTA_STATUS_NO_FOTA_SESSION        = bytearray(b'\x0b')

# FOTA op codes
FOTA_OP_CODE_START                 = bytearray(b'\x01')
FOTA_OP_CODE_STOP                  = bytearray(b'\x02')
FOTA_OP_CODE_COMMIT                = bytearray(b'\x03')
FOTA_OP_CODE_SET_XOFF              = bytearray(b'\x41')
FOTA_OP_CODE_SET_XON               = bytearray(b'\x42')
FOTA_OP_CODE_SET_FRAGMENT_ID       = bytearray(b'\x43')

# Number of bytes in a single BSC packet, excluding fragment ID
FRAGMENT_SIZE       = 128

# Max. Frag. ID before overflow to zero
MAXIMUM_FRAGMENT_ID = 255


def get_chunk_n(data, chunk_size: int, n: int):
    start = chunk_size * n
    end = chunk_size * (n+1)
    if len(data) <= start:
        return []
    if len(data) <= end:
        return data[start:]

    return data[start:end]


async def find_device(name: str) -> Optional[BleakClient]:
    logger.info(f"Scanning for device '{name}'")
    devices = await BleakScanner.discover()
    for d in devices:
        if name == d.name:
            logger.info(f'Found it. Address = {d.address}')
            client = BleakClient(d)
            count = 0
            while count < 5:
                try:
                    logger.info(f'Connecting to BLE device @ {d.address}')
                    connected = await client.connect()
                    if connected is True:
                        return client
                except BleakError:
                    logger.info(f'Waiting to connect: ({count}/{5})')
                    pass
                count += 1
            logger.error(f'Failed to connect to BLE device @ {d.address}')
            return None
    logger.error(f'Failed to find device named {name}')
    return None


class StatusNotificationHandler:

    def __init__(self):
        self.status_value = bytearray()
        self.status_event = asyncio.Event()

    def handle_status_notification(self, char_handle: int, data: bytearray):
        logger.info(f"Status notification: {''.join('{:02x}'.format(x) for x in data)}")
        self.status_value = data
        self.status_event.set()

    async def wait_for_status_notification(self):
        await self.status_event.wait()
        self.status_event.clear()


class FOTASession:

    def __init__(self, client: BleakClient, data: bytes):
        self.client = client
        self.data = data
        self.handler = StatusNotificationHandler()
        self.fragment_id = 0
        self.rollover_counter = 0
        self.flow_paused = False

    def update_fragment_id(self, fragment_id):
        # Account for rollover
        if fragment_id > self.fragment_id:
            self.rollover_counter -= 1
        self.fragment_id = fragment_id
        if  self.rollover_counter < 0:
            self.rollover_counter = 0
        if  self.fragment_id < 0:
            self.fragment_id = 0

    def check_status_notifications(self):
        if self.handler.status_event.is_set():
            status = bytearray([self.handler.status_value[0]])
            if status == FOTA_STATUS_OK:
                logger.info(f'Received status OK')
            elif status == FOTA_STATUS_XOFF:
                fragment_id = self.handler.status_value[1]
                logger.info(f'Received status XOFF. '
                            f'Fragment ID = {fragment_id}')
                self.flow_paused = True
                self.update_fragment_id(fragment_id)
            elif status == FOTA_STATUS_XON:
                fragment_id = self.handler.status_value[1]
                logger.info(f'Received status XON. '
                            f'Fragment ID = {fragment_id}')
                self.flow_paused = False
                self.update_fragment_id(fragment_id)
            elif status == FOTA_STATUS_SYNC_LOST:
                fragment_id = self.handler.status_value[1]
                logger.info(f'Received status SYNC_LOST. '
                            f'Fragment ID = {fragment_id}')
                self.update_fragment_id(fragment_id)
            self.handler.status_event.clear()

    async def get_firmware_revision(self) -> (str, Union[str, None]):
        """
        Gets the firmware revision from the optional Firmware Revision String characteristic of the DFU Service
        or from the Gatt Server's Device Information Service.

        If the Firmware Revision String characteristic exists on the DFU Service, it may have an optional
        Characteristic User Description descriptor (CUDD) that uniquely identifies which embedded device the
        target firmware is executed on. This is required if the Gatt Server has multiple DFU Service instances

        :return: tuple (fw_rev, device_description)
        """
        fota_svc = self.client.services.get_service(UUID_FOTA_SERVICE)
        fw_rev_char = fota_svc.get_characteristic(UUID_FIRMWARE_REVISION_STRING_CHAR)
        fw_rev_desc_str = None
        if fw_rev_char:
            fw_rev_str = await self.client.read_gatt_char(fw_rev_char)
            fw_rev_desc = fw_rev_char.get_descriptor(UUID_CUDD)
            if fw_rev_desc:
                fw_rev_desc_str = await self.client.read_gatt_descriptor(fw_rev_desc.handle)
        else:
            dev_info_svc = self.client.services.get_service(UUID_DEVICE_INFORMATION_SERVICE)
            fw_rev_char = dev_info_svc.get_characteristic(UUID_FIRMWARE_REVISION_STRING_CHAR)
            fw_rev_str = await self.client.read_gatt_char(fw_rev_char)

        return fw_rev_str, fw_rev_desc_str

    async def start(self):
        # Subscribe to notifications from the status characteristic
        await self.client.start_notify(UUID_STATUS_CHAR, self.handler.handle_status_notification)

        # Start a FOTA session
        await self.client.write_gatt_char(UUID_CONTROL_CHAR, FOTA_OP_CODE_START, True)

        # Wait for the client to write XON to the status characteristic
        count = 0
        while count < 5:
            try:
                await asyncio.wait_for(self.handler.wait_for_status_notification(), timeout=10.0)
                status = bytearray([self.handler.status_value[0]])
                if status == FOTA_STATUS_XON:
                    logger.info(f'Received status XON')
                    break
                elif status == FOTA_STATUS_XOFF:
                    logger.info(f'Received status XOFF')
                else:
                    logger.warning(f'Received unknown status: {status}')
            except asyncio.TimeoutError:
                logger.info(f'Waiting for status notification: ({count}/{5})')
                count += 1
                if count >= 5:
                    raise asyncio.TimeoutError

    async def transfer_binary(self):
        binary_sent = False

        while not binary_sent:
            self.check_status_notifications()

            if self.flow_paused:
                continue

            n = (MAXIMUM_FRAGMENT_ID + 1) * self.rollover_counter + self.fragment_id
            packet_data = bytearray(get_chunk_n(self.data, FRAGMENT_SIZE, n))
            if len(packet_data) < FRAGMENT_SIZE:
                binary_sent = True
                if len(packet_data) == 0:
                    continue
            fragment = bytearray([self.fragment_id]) + packet_data
            try:
                await asyncio.wait_for(self.client.write_gatt_char(UUID_BINARY_STREAM_CHAR, fragment, False),
                                       timeout=0.025)
                await asyncio.sleep(0.075)
            except asyncio.TimeoutError:
                logger.error(f'Timeout error occurred while writing BSC')
                await asyncio.sleep(0.100)

            self.fragment_id += 1
            # In case of rollover, increment counter and reset fragment ID
            if  self.fragment_id >= MAXIMUM_FRAGMENT_ID + 1:
                self.rollover_counter += 1
                self.fragment_id = 0

        await self.client.write_gatt_char(UUID_CONTROL_CHAR, FOTA_OP_CODE_COMMIT, True)


async def main():
    client = await find_device('FOTA')

    if client is None:
        return

    def disconnected_callback(c: BleakClient):
        disconnection_complete_event.set()

    async def waiter(event: asyncio.Event):
        await event.wait()

    client.set_disconnected_callback(disconnected_callback)
    disconnection_complete_event = asyncio.Event()
    disconnection_complete = asyncio.create_task(waiter(disconnection_complete_event))

    path = input('Enter the path to the binary: ')

    try:
        file = open(path, 'rb')
    except IOError:
        logger.error(f'Could not open file {path}')
        await client.disconnect()
        raise
    data = file.read()

    session = FOTASession(client, data)

    fw_rev, dev_str = await session.get_firmware_revision()
    logger.info(f'DFU Service found with firmware rev {fw_rev.decode("utf-8")}' +
               (f' for device "{dev_str.decode("utf-8")}"' if dev_str else ''))

    try:
        await session.start()
    except asyncio.TimeoutError:
        logger.error(f'FOTA session failed to start within timeout period')
        await client.disconnect()
        return

    logger.info(f'FOTA session started')

    logger.info(f'Transferring binary of size {len(data)} bytes, '
                f'hash: {hashlib.sha256(data).hexdigest()}')

    await session.transfer_binary()

    await disconnection_complete

    logger.info("FOTA session complete, waiting for device to apply update")
    await asyncio.sleep(60.0)

    client = await find_device('FOTA')

    if client is None:
        return

    session = FOTASession(client, data)

    new_fw_rev, new_dev_str = await session.get_firmware_revision()
    logger.info(f'DFU Service found with firmware rev {new_fw_rev.decode("utf-8")}' +
               (f' for device "{new_dev_str.decode("utf-8")}"' if new_dev_str else ''))

    if new_fw_rev != fw_rev:
        logger.info("Update successful!")
    else:
        logger.info("Update unsuccessful :(")

    await client.disconnect()

if __name__ == '__main__':
    loop = asyncio.get_event_loop()
    loop.run_until_complete(main())
