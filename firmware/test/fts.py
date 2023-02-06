import gatt
import logging
import time
import json

"""
This class represents a set of methods implementing a simple FTS client. 
It allows to:
- get list of files available on the device
- get information about any of the files on the device
- get the file contents (from device to the user)
- get status of the filesystem (free/occupied space, files' count)
"""
class FtsClient:
    file_transfer_service_uuid = "a0451001-b822-4820-8782-bd8faf68807b"

    fts_cp_char_uuid =           "00001002-0000-1000-8000-00805f9b34fb"
    fts_file_list_char_uuid =    "00001003-0000-1000-8000-00805f9b34fb"
    fts_file_info_char_uuid =    "00001004-0000-1000-8000-00805f9b34fb"
    fts_file_data_char_uuid =    "00001005-0000-1000-8000-00805f9b34fb"
    fts_fs_status_char_uuid =    "00001006-0000-1000-8000-00805f9b34fb"
    fts_status_char_uuid =       "00001007-0000-1000-8000-00805f9b34fb"

    values = {}
    pending_flags = {}

    def __init__(self, dictofun):
        self.cp_char = dictofun.get_characteristic_by_uuid(self.fts_cp_char_uuid)
        self.list_char = dictofun.get_characteristic_by_uuid(self.fts_file_list_char_uuid)
        self.info_char = dictofun.get_characteristic_by_uuid(self.fts_file_info_char_uuid)
        self.data_char = dictofun.get_characteristic_by_uuid(self.fts_file_data_char_uuid)
        self.fs_status = dictofun.get_characteristic_by_uuid(self.fts_fs_status_char_uuid)
        self.status = dictofun.get_characteristic_by_uuid(self.fts_status_char_uuid)
        if self.cp_char is None or self.list_char is None or self.data_char is None or self.fs_status is None or self.status is None:
            raise Exception("failed to access FTS characteristic")
        self.status.enable_notifications()
        self.dictofun = dictofun

    ##################### Implementation of GATT callbacks #################

    def characteristic_value_updated(self, characteristic, value):
        #logging.debug(" char [%s] updated" % (characteristic.uuid))
        self.values[characteristic.uuid] = value
        self.pending_flags[characteristic.uuid] = True
        if characteristic.uuid == self.status.uuid:
            logging.warn(" status char updated. new value: %s " + str(value))

    def characteristic_read_value_failed(self, characteristic, error):
        logging.debug(" char [%s] read value failed (%s)" % (characteristic.uuid, str(error)))
        pass

    def characteristic_write_value_succeeded(self, characteristic):
        #logging.debug(" char [%s] write value success" % (characteristic.uuid))
        pass

    def characteristic_write_value_failed(self, characteristic, error):
        logging.info(" char [%s] write value failed (%s)" % (characteristic.uuid, str(error)))
        pass

    def characteristic_enable_notifications_succeeded(self, characteristic):
        #logging.debug(" char [%s] enabled notifications successfully" % (characteristic.uuid))
        pass

    def characteristic_enable_notifications_failed(self, characteristic, error):
        logging.debug(" char [%s] enable notifications failed failed (0x%s)" % (characteristic.uuid, str(error)))
        pass
    
    ##################### Implementation of requests #################
    # Opcodes 1-4: you can find explanations in the README.md: https://github.com/rundb/dictofun/tree/master/firmware/src/lib/ble_fts#control-point
    def _request_files_list(self):
        self.cp_char.write_value(bytearray([1]))

    def _request_file_info(self, file_id):
        request = bytearray([2])
        file_id_bytes = file_id.to_bytes(8, "little")
        for b in file_id_bytes:
            request.append(b)
        self.cp_char.write_value(request)

    def _request_file_data(self, file_id):
        request = bytearray([3])
        file_id_bytes = file_id.to_bytes(8, "little")
        for b in file_id_bytes:
            request.append(b)
        self.cp_char.write_value(request)

    def _request_fs_status(self):
        self.cp_char.write_value(bytearray([4]))
    
    ##################### Implementation of parsers #################
    def _parse_files_count(self, array):
        a = array
        return int(a[0] + (a[1] << 8) + (a[2] << 16) + (a[3] << 24))

    def _parse_files_list(self, array):
        element_size = 8
        elements = []
        idx = 0
        while len(elements) < len(array) / element_size:
            elements.append(int.from_bytes(array[idx:idx + element_size], "little"))
            idx += element_size
        return elements

    def _parse_json_size(self, array):
        a = array
        return int(a[0] + (a[1] << 8))

    def _parse_file_info_json(self, json_value):
        file_info = {}
        try:
            metadata = json.loads(json_value)
            file_info["size"] = metadata["s"]
            file_info["frequency"] = metadata["f"]
            file_info["codec"] = metadata["c"]
        except Exception as e:
            logging.error("JSON decoding error " + str(e))
            return file_info
        return file_info

    def _parse_fs_status(self, raw):
        fs_status = {}
        fs_status["free_space"] = int.from_bytes(raw[2:5], "little")
        fs_status["occupied_space"] = int.from_bytes(raw[6:9], "little")
        fs_status["count"] = int.from_bytes(raw[10:13], "little")
        return fs_status

    ##################### Implementation of public API methods #################
    def get_files_list(self):
        self.list_char.enable_notifications()
        time.sleep(0.5)
        self.pending_flags[self.list_char.uuid] = False
        self._request_files_list()

        # minimal expected size, if there are no files provided
        expected_size = 8
        
        received_data = bytearray([])
        start_time = time.time()
        transaction_timeout = 10 # seconds

        while (not self.pending_flags[self.list_char.uuid]) and (time.time() - start_time < transaction_timeout):
            time.sleep(0.01)
        
        if self.pending_flags[self.list_char.uuid]:
            raw = self.values[self.list_char.uuid]
            self.pending_flags[self.list_char.uuid] = False
            received_data += raw
            expected_size = (self._parse_files_count(received_data) + 1) * 8
        else:
            logging.error("file list: response timeout. exiting routine with an empty result list")
            return []
        
        while len(received_data) != expected_size and time.time() - start_time < transaction_timeout:
            if self.pending_flags[self.list_char.uuid]:
                self.pending_flags[self.list_char.uuid] = False
                received_data += self.values[self.list_char.uuid]
        
        if time.time() - start_time >= transaction_timeout:
            logging.error("files list: transaction timeout")
            return []

        return self._parse_files_list(received_data[8:])

    def get_file_info(self, file_id):
        self.info_char.enable_notifications()
        time.sleep(0.5)
        self.pending_flags[self.info_char.uuid] = False
        self._request_file_info(file_id)

        expected_size = 8
        
        received_data = bytearray([])
        start_time = time.time()
        transaction_timeout = 10 # seconds

        while (not self.pending_flags[self.info_char.uuid]) and (time.time() - start_time < transaction_timeout):
            time.sleep(0.01)
        
        if self.pending_flags[self.info_char.uuid]:
            raw = self.values[self.info_char.uuid]
            self.pending_flags[self.info_char.uuid] = False
            received_data += raw
            expected_size = self._parse_json_size(received_data) + 2
        else:
            logging.error("file info: transaction timeout")
            return {}
        
        while len(received_data) != expected_size and time.time() - start_time < transaction_timeout:
            if self.pending_flags[self.info_char.uuid]:
                received_data += self.values[self.info_char.uuid]
                self.pending_flags[self.info_char.uuid] = False
        
        if time.time() - start_time >= transaction_timeout:
            logging.error("file info: transaction timeout. Received %d out of %d bytes" % (len(received_data), expected_size) )
            return []

        json_info = received_data[2:].decode("utf-8")
        return self._parse_file_info_json(json_info)

    def get_file_data(self, file_id, size):
        self.data_char.enable_notifications()
        self.pending_flags[self.data_char.uuid] = False
        self._request_file_data(file_id)
        expected_size = size
        
        received_data = bytearray([])
        start_time = time.time()
        transaction_timeout = 0.1 * size

        while len(received_data) != expected_size and time.time() - start_time < transaction_timeout:
            if self.pending_flags[self.data_char.uuid]:
                received_data += self.values[self.data_char.uuid]
                self.pending_flags[self.data_char.uuid] = False
        
        if time.time() - start_time >= transaction_timeout:
            logging.error("file info: transaction timeout. Received %d out of %d bytes" % (len(received_data), expected_size) )

        return received_data

    def get_fs_status(self):
        self.fs_status.enable_notifications(True)

        self._request_fs_status()
        self.pending_flags[self.fs_status.uuid] = False

        received_data = bytearray([])
        start_time = time.time()
        transaction_timeout = 2

        while (not self.pending_flags[self.fs_status.uuid]) and (time.time() - start_time < transaction_timeout):
            time.sleep(0.01)

        if time.time() - start_time >= transaction_timeout:
            logging.error("fs status: timeout detected")
            return {}
        
        raw = self.values[self.fs_status.uuid]
        fs_status = self._parse_fs_status(raw)
        
        return fs_status
