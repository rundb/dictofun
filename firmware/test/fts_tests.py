#from fts import DictofunFtsClient
import logging
import sys
import gatt
import threading
import time
from enum import Enum
from dictofun_ble import DictofunBle, DictofunDeviceManager

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

def launch_tests():
    pass

if __name__ == '__main__':
    configure_log()

    manager = DictofunDeviceManager(adapter_name='hci0')
    manager.start_discovery()

    thr = threading.Thread(target=launch_ble_manager, args=[manager])
    thr.start()
    if manager.wait_until_discovered() < 0:
        exit_routine(manager)
    dictofun = manager.get_dictofun()
    if not dictofun is None:
        dictofun.connect_to_device(reconnect_attempts=1)
        logging.debug("connecting")
        if dictofun.wait_until_connected() < 0:
            exit_routine(manager)
        if dictofun.wait_until_services_resolved() < 0:
            exit_routine(manager)

        launch_tests()

        dictofun.disconnect()
        logging.debug("disconnecting")
        if dictofun.wait_until_disconnected() < 0:
            exit_routine(manager)
        
        time.sleep(0.1)
    else:
        logging.error("no dictofun discovered")

    manager.stop()

