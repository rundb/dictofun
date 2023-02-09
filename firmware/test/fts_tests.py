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
        level=logging.INFO,
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
    
####################### Test functions (assuming that BLE subsystem runs in testing mode) ######
def test_files_list_getter(fts):
    # Execute test on reading out the list of the files available on the device
    files_list = fts.get_files_list()
    logging.debug("FTS Server provides following %d files" % len(files_list))
    for file in files_list:
        logging.debug("\t%x" % file)
    if len(files_list) != 2 or files_list[0] != 0x102030405060708:
        return -1

    return 0

def test_files_info_getter(fts):
    test_file_id = 0x102030405060708
    file_info = fts.get_file_info(test_file_id)
    if len(file_info) == 0 or file_info["size"] != 512:
        return -1
    return 0

def test_files_data_getter(fts):
    test_file_id = 0x102030405060708
    test_file_size = 512
    file_data = fts.get_file_data(test_file_id, test_file_size)
    if len(file_data) != 512:
        logging.error("received file data size = %d != 512" % len(file_data))
        return -1
    return 0

def test_fs_status_getter(fts):
    fs_status = fts.get_fs_status()
    if len(fs_status) == 0 or fs_status["count"] != 2:
        logging.error("failed to receive fs status")
        return -1
    return 0

####################### Test functions (assuming that BLE subsystem runs in target mode) ######
def check_files_list_getter(fts):
    files_list = fts.get_files_list()
    logging.info("Target FTS Server provides following %d files" % len(files_list))
    for file in files_list:
        logging.info("\t%x" % file)
    return files_list

def check_files_info_getter(fts, files):
    test_file_id = files[0]
    file_info = fts.get_file_info(test_file_id)
    logging.info("First file in the FS: %s" % str(file_info))
    

"""
Tests' launcher: executes all tests and prints their status to the log.
Also returns -1, if any of the tests failed (so return value can be used in CI automation scripting)
"""
def launch_tests(dictofun):
    fts = None
    try:
        fts = FtsClient(dictofun)
    except Exception as e:
        logging.error("failed to create FTS Client instance, error: %s" % str(e))
        exit(-1)

    files_list_result = test_files_list_getter(fts)
    if files_list_result < 0:
        logging.error("files list test: failed")
    else:
        logging.debug("files list test: passed")

    info_getter_result = test_files_info_getter(fts)
    if info_getter_result < 0:
        logging.error("file info test: failed")
    else:
        logging.debug("file info test: passed")

    data_getter_result = test_files_data_getter(fts)
    if data_getter_result < 0:
        logging.error("file data test: failed")
    else:
        logging.debug("file data test: passed")

    fs_status_getter_result = test_fs_status_getter(fts)
    if fs_status_getter_result < 0:
        logging.error("fs status test: failed")
    else:
        logging.debug("fs status test: passed")

    return min(0, files_list_result, info_getter_result, data_getter_result, fs_status_getter_result)


"""
This method should be run after switching the dictofun into target mode (thus connecting littlefs and FTS)
"""
def launch_checks(dictofun):
    fts = None
    try:
        fts = FtsClient(dictofun)
    except Exception as e:
        logging.error("failed to create FTS Client instance, error: %s" % str(e))
        exit(-1)

    files = check_files_list_getter(fts)
    check_files_info_getter(fts, files)

"""
Here all preparations (except for UART commands is performed)
TODO: consider placing here the `ble 1` command.

After this command is done FTS tests can be executed.
"""
def prepare_dictofun(dictofun, dictofun_control):
    global device_output
    dictofun.connect_to_device(reconnect_attempts=1)
    if dictofun.wait_until_connected() < 0:
        exit_routine(manager)
    if dictofun.wait_until_services_resolved() < 0:
        exit_routine(manager)
    device_output += dictofun_control.issue_command("\n", 0.1)


def release_dictofun(dictofun, dictofun_control):
    global device_output
    dictofun.disconnect()
    if dictofun.wait_until_disconnected() < 0:
        exit_routine(manager)
    device_output += dictofun_control.issue_command("\n", 0.1)


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
    device_output += dictofun_control.issue_command("ble 1", 0.5)

    manager = DictofunDeviceManager(adapter_name='hci0')
    manager.start_discovery()

    thr = threading.Thread(target=launch_ble_manager, args=[manager])
    thr.start()
    if manager.wait_until_discovered() < 0:
        device_output += dictofun_control.issue_command("\n", 0.1)
        logging.error("device output: " + str(device_output))
        exit_routine(manager)
    dictofun = manager.get_dictofun()
    
    test_execution_result = 0
    if not dictofun is None:
        prepare_dictofun(dictofun, dictofun_control)
        # test_execution_result = launch_tests(dictofun)
        dictofun_control.issue_command("ble 4", 0.5)
        launch_checks(dictofun)
        release_dictofun(dictofun, dictofun_control)
    else:
        logging.error("no dictofun discovered")

    manager.stop()

    device_output += dictofun_control.issue_command("\n", 0.5)

    if "error" in device_output:
        logging.error("found keyword `error` in the device output.")
        logging.error("device output: " + str(device_output))
        test_execution_result = -1
    else:
        logging.debug("device output: " + str(device_output))
    

    if test_execution_result < 0:
        logging.error("tests failed")
    else:
        logging.info("tests passed")
    exit(test_execution_result)
