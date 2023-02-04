import gatt
import logging

class FtsClient:
    file_transfer_service_uuid = "a0451001-b822-4820-8782-bd8faf68807b"

    fts_cp_char_uuid =           "00001002-0000-1000-8000-00805f9b34fb"
    fts_file_list_char_uuid =    "00001003-0000-1000-8000-00805f9b34fb"
    fts_file_info_char_uuid =    "00001004-0000-1000-8000-00805f9b34fb"
    fts_file_data_char_uuid =    "00001005-0000-1000-8000-00805f9b34fb"
    fts_fs_status_char_uuid =    "00001006-0000-1000-8000-00805f9b34fb"
    fts_status_char_uuid =       "00001007-0000-1000-8000-00805f9b34fb"

    def __init__(self, dictofun):
        self.cp_char = dictofun.get_characteristic_by_uuid(self.fts_cp_char_uuid)
        self.list_char = dictofun.get_characteristic_by_uuid(self.fts_file_list_char_uuid)
        self.info_char = dictofun.get_characteristic_by_uuid(self.fts_file_info_char_uuid)
        self.data_char = dictofun.get_characteristic_by_uuid(self.fts_file_data_char_uuid)
        self.fs_status = dictofun.get_characteristic_by_uuid(self.fts_fs_status_char_uuid)
        self.status = dictofun.get_characteristic_by_uuid(self.fts_status_char_uuid)
        self.status.enable_notifications()
        self.dictofun = dictofun

    def characteristic_value_updated(self, characteristic, value):
        pass

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