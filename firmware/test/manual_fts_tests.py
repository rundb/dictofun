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
    
####################### Test functions (assuming that BLE subsystem runs in testing mode) ######
def test_files_list_getter(fts):
    # Execute test on reading out the list of the files available on the device
    files_list = fts.get_files_list()
    logging.debug("FTS Server provides following %d files" % len(files_list))
    for file in files_list:
        logging.debug("\t%x" % file)

    return 0


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

    # info_getter_result = test_files_info_getter(fts)
    # if info_getter_result < 0:
    #     logging.error("file info test: failed")
    # else:
    #     logging.debug("file info test: passed")

    # data_getter_result = test_files_data_getter(fts)
    # if data_getter_result < 0:
    #     logging.error("file data test: failed")
    # else:
    #     logging.debug("file data test: passed")

    # fs_status_getter_result = test_fs_status_getter(fts)
    # if fs_status_getter_result < 0:
    #     logging.error("fs status test: failed")
    # else:
    #     logging.debug("fs status test: passed")

    return min(0, files_list_result)#, info_getter_result, data_getter_result, fs_status_getter_result)

"""
Here all preparations (except for UART commands is performed)
TODO: consider placing here the `ble 1` command.

After this command is done FTS tests can be executed.
"""
def prepare_dictofun(dictofun):
    dictofun.connect_to_device(reconnect_attempts=1)
    if dictofun.wait_until_connected() < 0:
        exit_routine(manager)
    time.sleep(1)
    if dictofun.wait_until_services_resolved() < 0:
        exit_routine(manager)


def release_dictofun(dictofun, dictofun_control):
    dictofun.disconnect()
    if dictofun.wait_until_disconnected() < 0:
        exit_routine(manager)


if __name__ == '__main__':
    configure_log()

    manager = DictofunDeviceManager(adapter_name='hci0')
    manager.start_discovery()

    thr = threading.Thread(target=launch_ble_manager, args=[manager])
    thr.start()
    if manager.wait_until_discovered() < 0:
        exit_routine(manager)
    dictofun = manager.get_dictofun()
    
    test_execution_result = 0
    if not dictofun is None:
        prepare_dictofun(dictofun)
        test_execution_result = launch_tests(dictofun)
        release_dictofun(dictofun)
    else:
        logging.error("no dictofun discovered")

    manager.stop()

