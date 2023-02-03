import gatt
import time
import logging
import sys
from dictofun_control import DictofunControl
import threading
from multiprocessing import Process
import json

dut_mac = "e3:50:c2:d4:e1:b9"

nordic_led_service_uuid =    "00001525-1212-efde-1523-785feabcd123"
file_transfer_service_uuid = "a0451001-b822-4820-8782-bd8faf68807b"
# phone shows start with a0451001
fts_cp_char_uuid =           "00001002-0000-1000-8000-00805f9b34fb"
fts_file_list_char_uuid =    "00001003-0000-1000-8000-00805f9b34fb"
fts_file_info_char_uuid =    "00001004-0000-1000-8000-00805f9b34fb"
fts_file_data_char_uuid =    "00001005-0000-1000-8000-00805f9b34fb"

def configure_log():
    # timestr = time.strftime("%Y%m%d-%H%M%S")
    # log_filename = "ble_test_" + timestr + ".log"
    log_filename = "ble_debug.log"
    logging.basicConfig(
        level=logging.DEBUG,
        format="%(asctime)s [%(levelname)s] %(message)s",
        handlers=[
            # logging.FileHandler(log_filename),
            logging.StreamHandler(sys.stdout)
        ]
    )


class DictofunBle(gatt.Device):
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


"""
This class aggregates methods of communication with BLE FTS Server
"""
class FtsClient:
    def __init__(self, dictofun):
        self.cp_char = dictofun.get_characteristic_by_uuid(fts_cp_char_uuid)
        self.list_char = dictofun.get_characteristic_by_uuid(fts_file_list_char_uuid)
        self.info_char = dictofun.get_characteristic_by_uuid(fts_file_info_char_uuid)
        self.data_char = dictofun.get_characteristic_by_uuid(fts_file_data_char_uuid)
        self.dictofun = dictofun

    def request_files_list(self):
        self.cp_char.write_value(bytearray([1]))

    def request_file_info(self, file_id):
        request = bytearray([2])
        file_id_bytes = file_id.to_bytes(8, "little")
        for b in file_id_bytes:
            request.append(b)
        self.cp_char.write_value(request)

    def request_file_data(self, file_id):
        request = bytearray([3])
        file_id_bytes = file_id.to_bytes(8, "little")
        for b in file_id_bytes:
            request.append(b)
        self.cp_char.write_value(request)

    def parse_files_count(self, array):
        a = array
        return int(a[0] + (a[1] << 8) + (a[2] << 16) + (a[3] << 24))

    def parse_json_size(self, array):
        a = array
        return int(a[0] + (a[1] << 8))

    def parse_files_list(self, array):
        element_size = 8
        elements = []
        idx = 0
        while len(elements) < len(array) / element_size:
            elements.append(int.from_bytes(array[idx:idx + element_size], "little"))
            idx += element_size
        return elements

    def get_file_size_from_json(self, raw_metadata):
        try:
            metadata = json.loads(raw_metadata)
            return int(metadata["s"])
        except Exception as e:
            print("JSON decoding error " + str(e))
            return 0
    
    def get_files_list(self):
        # caller has checked that chars exist.
        self.list_char.enable_notifications()
        time.sleep(0.5)
        self.request_files_list()
        is_first_packet_received = False

        # minimal expected size, if there are no files provided
        expected_size = 8
        
        received_data = bytearray([])
        start_time = time.time()
        transaction_timeout = 10 # seconds

        while (not is_first_packet_received) and (time.time() - start_time < transaction_timeout):
            is_first_packet_received = self.dictofun.is_updated_value_pending
        
        if is_first_packet_received:
            raw = self.dictofun.get_last_received_packet()
            received_data += raw
            expected_size = (self.parse_files_count(received_data) + 1) * 8
        
        while len(received_data) != expected_size and time.time() - start_time < transaction_timeout:
            if self.dictofun.is_updated_value_pending:
                received_data += self.dictofun.get_last_received_packet()
        
        if time.time() - start_time >= transaction_timeout:
            logging.error("files list: transaction timeout")
            return []

        return self.parse_files_list(received_data[8:])


    def get_file_info(self, file_id):
        self.info_char.enable_notifications()
        time.sleep(0.5)
        self.request_file_info(file_id)

        is_first_packet_received = False

        # minimal expected size, if there are no files provided
        expected_size = 8
        
        received_data = bytearray([])
        start_time = time.time()
        transaction_timeout = 10 # seconds

        while (not is_first_packet_received) and (time.time() - start_time < transaction_timeout):
            is_first_packet_received = self.dictofun.is_updated_value_pending
        
        if is_first_packet_received:
            raw = self.dictofun.get_last_received_packet()
            received_data += raw
            expected_size = self.parse_json_size(received_data) + 2
        
        while len(received_data) != expected_size and time.time() - start_time < transaction_timeout:
            if self.dictofun.is_updated_value_pending:
                received_data += self.dictofun.get_last_received_packet()
        
        if time.time() - start_time >= transaction_timeout:
            logging.error("file info: transaction timeout. Received %d out of %d bytes" % (len(received_data), expected_size) )
            return []

        return received_data[2:].decode("utf-8")

    def get_file_data(self, file_id, size):
        # reset possibly pending previous transactions
        self.dictofun.get_last_received_packet()

        self.info_char.enable_notifications(False)
        self.list_char.enable_notifications(False)
        time.sleep(0.5)

        self.data_char.enable_notifications()
        time.sleep(1)
        self.dictofun.get_last_received_packet()
        time.sleep(0.5)

        self.request_file_data(file_id)

        is_first_packet_received = False

        # minimal expected size, if there are no files provided
        expected_size = size
        
        received_data = bytearray([])
        start_time = time.time()
        transaction_timeout = 10 # seconds
        while (not is_first_packet_received) and (time.time() - start_time < transaction_timeout):
            is_first_packet_received = self.dictofun.is_updated_value_pending
        
        if is_first_packet_received:
            raw = self.dictofun.get_last_received_packet()
            received_data += raw
        else:
            logging.error("timeout error")
            return []
        
        while len(received_data) != expected_size and time.time() - start_time < transaction_timeout:
            if self.dictofun.is_updated_value_pending:
                received_data += self.dictofun.get_last_received_packet()
        
        if time.time() - start_time >= transaction_timeout:
            logging.error("file info: transaction timeout. Received %d out of %d bytes" % (len(received_data), expected_size) )

        return received_data


class AnyDeviceManager(gatt.DeviceManager):
    mac_address = ""
    dictofun = None
    def device_discovered(self, device):
        if "dict" in device.alias():
            logging.info(device.alias() + " " + str(device.mac_address))
            if not device.is_connected():
                self.mac_address = device.mac_address
                self.dictofun = DictofunBle(mac_address = device.mac_address, manager = self)
                self.dictofun.connect()
                time.sleep(0.5)
                self.dictofun.services_resolved()
                time.sleep(2)

    def get_dictofun(self):
        return self.dictofun


def launch_ble_manager(manager):
    manager.run()


def run_led_control_tests(dictofun):
    if dictofun.get_characteristic_by_uuid(nordic_led_service_uuid) is None:
        logging.error("failed to read LED characteristic from the device")
        return -1

    # write to the LED characteristic to enable LED
    dictofun.get_characteristic_by_uuid(nordic_led_service_uuid).write_value(bytearray([1]))
    time.sleep(1)
    # write to the LED characteristic to disable LED
    dictofun.get_characteristic_by_uuid(nordic_led_service_uuid).write_value(bytearray([0]))
    time.sleep(0.2)

    return 0


def run_fts_tests(dictofun):
    if dictofun.get_characteristic_by_uuid(fts_cp_char_uuid) is None:
        logging.error("failed to read FTS characteristic from the device")
        return -1        
    
    fts_client = FtsClient(dictofun)

    # Execute test on reading out the list of the files available on the device
    files_list = fts_client.get_files_list()
    logging.info("FTS Server provides following %d files" % len(files_list))
    for file in files_list:
        logging.info("\t%x" % file)
    
    file0_info = ""
    if len(files_list) > 0:
        file0_info = fts_client.get_file_info(files_list[0])
        logging.info("\tfile 0 info: %s" % file0_info)
    
    if len(file0_info) == 0:
        logging.error("failed to fetch file info (empty json received)")
        return -1
    
    file_size = fts_client.get_file_size_from_json(file0_info)
    if file_size == 0:
        logging.error("File size from JSON is 0. Aborting")
        return -1

    file0_data = fts_client.get_file_data(files_list[0], file_size)

    logging.info("received file with size %d" % len(file0_data))
    return 0


if __name__ == '__main__':
    configure_log()

    device_control = DictofunControl("/dev/ttyUSB0")
    reset_result = device_control.issue_command("reset", 0.5)
    device_output = ""
    device_output += reset_result

    ble_enable_result = device_control.issue_command("bletest 1", 0.2)
    device_output += ble_enable_result

    manager = AnyDeviceManager(adapter_name='hci0')
    manager.start_discovery()

    thr = threading.Thread(target=launch_ble_manager, args=[manager])
    thr.start()
    time.sleep(3)

    trigger_output_result = device_control.issue_command("\n")
    device_output += trigger_output_result

    dictofun = manager.get_dictofun()

    led_test = run_led_control_tests(dictofun)
    fts_test = run_fts_tests(dictofun)

    dictofun.disconnect()
    manager.stop()

    time.sleep(0.5)
    trigger_output_result = device_control.issue_command("\n")
    device_output += trigger_output_result

    logging.info("device output from the test execution:\n" + device_output)

    if led_test != 0 or fts_test != 0:
        exit(-1)
