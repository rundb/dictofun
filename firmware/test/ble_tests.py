import gatt
import time
import logging
import sys
from dictofun_control import DictofunControl
import threading
from multiprocessing import Process

dut_mac = "e3:50:c2:d4:e1:b9"

nordic_led_service_uuid =    "00001525-1212-efde-1523-785feabcd123"
file_transfer_service_uuid = "a0451001-b822-4820-8782-bd8faf68807b"
# phone shows start with a0451001
fts_cp_char_uuid =           "00001002-0000-1000-8000-00805f9b34fb"
fts_file_list_char_uuid =    "00001003-0000-1000-8000-00805f9b34fb"

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
                time.sleep(1)
                self.dictofun.services_resolved()
                time.sleep(0.3)

    def get_dictofun(self):
        return self.dictofun


def launch_ble_manager(manager):
    manager.run()


def run_led_control_tests(dictofun):
    # write to the LED characteristic to enable LED
    dictofun.get_characteristic_by_uuid(nordic_led_service_uuid).write_value(bytearray([1]))
    time.sleep(1)
    # write to the LED characteristic to disable LED
    dictofun.get_characteristic_by_uuid(nordic_led_service_uuid).write_value(bytearray([0]))
    time.sleep(0.2)


def run_fts_tests(dictofun):
    # request files' list
    dictofun.get_characteristic_by_uuid(fts_cp_char_uuid).write_value(bytearray([1]))
    # TODO: implement readback from files' list char here and check the results


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

    run_led_control_tests(dictofun)
    run_fts_tests(dictofun)

    dictofun.disconnect()
    manager.stop()

    time.sleep(1)
    trigger_output_result = device_control.issue_command("\n")
    device_output += trigger_output_result

    logging.info("device output from the test execution:\n" + device_output)