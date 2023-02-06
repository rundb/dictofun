import gatt
import logging
import time

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
        #self.status.enable_notifications()
        self.dictofun = dictofun

    def characteristic_value_updated(self, characteristic, value):
        logging.debug(" char [%s] updated" % (characteristic.uuid))
        self.values[characteristic.uuid] = value
        self.pending_flags[characteristic.uuid] = True

    def is_new_packet_pending(self, characteristic):
        return self.pending_flags[characteristic.uuid]

    def characteristic_read_value_failed(self, characteristic, error):
        logging.debug(" char [%s] read value failed (%s)" % (characteristic.uuid, str(error)))
        pass

    def characteristic_write_value_succeeded(self, characteristic):
        logging.debug(" char [%s] write value success" % (characteristic.uuid))
        pass

    def characteristic_write_value_failed(self, characteristic, error):
        logging.info(" char [%s] write value failed (%s)" % (characteristic.uuid, str(error)))
        pass

    def characteristic_enable_notifications_succeeded(self, characteristic):
        logging.debug(" char [%s] enabled notifications successfully" % (characteristic.uuid))
        pass

    def characteristic_enable_notifications_failed(self, characteristic, error):
        logging.debug(" char [%s] enable notifications failed failed (0x%s)" % (characteristic.uuid, str(error)))
        pass
    
    def _request_files_list(self):
        self.cp_char.write_value(bytearray([1]))
    
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

        while (not self.is_new_packet_pending(self.list_char)) and (time.time() - start_time < transaction_timeout):
            pass
        
        if self.is_new_packet_pending(self.list_char):
            raw = self.values[self.list_char.uuid]
            self.pending_flags[self.list_char.uuid] = False
            received_data += raw
            expected_size = (self._parse_files_count(received_data) + 1) * 8
        else:
            logging.error("file list: response timeout. exiting routine with an empty result list")
            return []
        
        while len(received_data) != expected_size and time.time() - start_time < transaction_timeout:
            if self.is_new_packet_pending(self.list_char):
                self.pending_flags[self.list_char.uuid] = False
                received_data += self.values[self.list_char.uuid]
        
        if time.time() - start_time >= transaction_timeout:
            logging.error("files list: transaction timeout")
            return []

        return self._parse_files_list(received_data[8:])
