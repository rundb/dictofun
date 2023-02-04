import gatt
import logging

# TODO: rename after refactoring completion
class DictofunFtsClient(gatt.Device):
    file_transfer_service_uuid = "a0451001-b822-4820-8782-bd8faf68807b"

    fts_cp_char_uuid =           "00001002-0000-1000-8000-00805f9b34fb"
    fts_file_list_char_uuid =    "00001003-0000-1000-8000-00805f9b34fb"
    fts_file_info_char_uuid =    "00001004-0000-1000-8000-00805f9b34fb"
    fts_file_data_char_uuid =    "00001005-0000-1000-8000-00805f9b34fb"
    fts_fs_status_char_uuid =    "00001006-0000-1000-8000-00805f9b34fb"
    fts_status_char_uuid =       "00001007-0000-1000-8000-00805f9b34fb"

    def __init__(self):

        pass

    def connect_succeeded(self):
        super().connect_succeeded()
        logging.debug("[%s] Connected" % (self.mac_address))

    def connect_failed(self, error):
        super().connect_failed(error)
        logging.debug("[%s] Connection failed: %s" % (self.mac_address, str(error)))

    def disconnect_succeeded(self):
        super().disconnect_succeeded()
        logging.debug("[%s] Disconnected" % (self.mac_address))

    def services_resolved(self):
        self.is_updated_value_pending = False
        super().services_resolved()

        logging.debug("[%s] Resolved services" % (self.mac_address))
        for service in self.services:
            logging.debug("[%s]  Service [%s]" % (self.mac_address, service.uuid))
            for characteristic in service.characteristics:
                logging.debug("[%s]    Characteristic [%s]" % (self.mac_address, characteristic.uuid))

    def get_characteristic_by_uuid(self, uuid):
        for service in self.services:
            for characteristic in service.characteristics:
                if str(characteristic.uuid).capitalize() == str(uuid).capitalize():
                    return characteristic
        return None

    def characteristic_value_updated(self, characteristic, value):
        # logging.debug(" char [%s] value updated" % (characteristic.uuid))
        # if str(characteristic.uuid) == fts_status_char_uuid:
        #     logging.info("detected status update in the target: %s" + str(value))
        # else:
        self.is_updated_value_pending = True
        self.value = value
    
    def get_last_received_packet(self):
        self.is_updated_value_pending = False
        return self.value

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