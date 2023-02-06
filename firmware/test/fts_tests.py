#from fts import DictofunFtsClient
import logging
import sys
import gatt
import threading
import time
from enum import Enum
from dictofun_ble import DictofunBle, DictofunDeviceManager
from dictofun_control import DictofunControl
from fts import FtsClient

device_output = ""

def configure_log():
    logging.basicConfig(
        level=logging.DEBUG,
        format="%(asctime)s [%(levelname)s] %(message)s",
        handlers=[
            logging.StreamHandler(sys.stdout)
        ]
    )

"""
Function call `manager.run()` has to be launched asynchronously, as it doesn't have 
any kind of timeout parameter to stop synchronously. 
"""
def launch_ble_manager(manager):
    manager.run()

def exit_routine(manager):
    manager.stop()
    exit(-1)

def launch_tests(dictofun):
    fts = None
    try:
        fts = FtsClient(dictofun)
    except Exception as e:
        logging.error("failed to create FTS Client instance, error: %s" % str(e))
        exit(-1)

    # Execute test on reading out the list of the files available on the device
    files_list = fts.get_files_list()
    logging.info("FTS Server provides following %d files" % len(files_list))
    for file in files_list:
        logging.info("\t%x" % file)
    
    return 0

def prepare_dictofun(dictofun, dictofun_control):
    global device_output
    dictofun.connect_to_device(reconnect_attempts=1)
    logging.debug("connecting")
    if dictofun.wait_until_connected() < 0:
        exit_routine(manager)
    if dictofun.wait_until_services_resolved() < 0:
        exit_routine(manager)

def release_dictofun(dictofun, dictofun_control):
    dictofun.disconnect()
    logging.debug("disconnecting")
    if dictofun.wait_until_disconnected() < 0:
        exit_routine(manager)
    time.sleep(0.1)


if __name__ == '__main__':
    configure_log()

    if len(sys.argv) != 2:
        logging.error("Wrong script syntax. Should have an argument (serial port)")
        exit(-1)
    serial_port = sys.argv[1]
    try:
        dictofun_control = DictofunControl(serial_port)
    except Exception as e:
        logging.error("failed to establish device control over serial port. Error: %s" % str(e))
        exit(-1)

    device_output += dictofun_control.issue_command("reset", 0.5)
    device_output += dictofun_control.issue_command("bletest 1", 0.5)

    manager = DictofunDeviceManager(adapter_name='hci0')
    manager.start_discovery()

    thr = threading.Thread(target=launch_ble_manager, args=[manager])
    thr.start()
    if manager.wait_until_discovered() < 0:
        device_output += dictofun_control.issue_command("\n", 0.1)
        logging.error("device output: " + str(device_output))
        exit_routine(manager)
    dictofun = manager.get_dictofun()
    
    if not dictofun is None:
        prepare_dictofun(dictofun, dictofun_control)
        launch_tests(dictofun)
        release_dictofun(dictofun, dictofun_control)
    else:
        logging.error("no dictofun discovered")

    device_output += dictofun_control.issue_command("\n", 0.5)

    logging.info("device output: " + str(device_output))
    #TODO: add parsing of device_output to discover errors

    manager.stop()

