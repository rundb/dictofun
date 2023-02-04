import logging
import gatt
from enum import Enum
import time
import threading

class DictofunBle(gatt.Device):
    mac_address = None
    manager = None
    reconnect_attempts = 0
    reconnect_time = 1
    are_services_resolved = False
    reconnect_thread = None

    def __init__(self, mac_address, manager):
        super().__init__(mac_address, manager)

    @classmethod
    def from_parent(cls, parent):
        return cls(parent.mac_address, parent.manager)

    """
    Due to inheritance issues connect() and _connect() can't be used here
    """
    def connect_to_device(self, reconnect_attempts = 0):
        self.reconnect_attempts = reconnect_attempts + 1
        self._connect_to_device()

    def _connect_to_device(self):
        if self.reconnect_attempts > 0:
            super().connect()
            self.reconnect_attempts -= 1

    def wait_until_connected(self, timeout = 5):
        start_time = time.time()
        timeout_value = timeout
        while not self.is_connected() and time.time() - start_time < timeout_value:
            time.sleep(0.1)
        if time.time() - start_time > timeout_value:
            logging.error("connection failed")
            return -1
        return 0
    
    def wait_until_disconnected(self, timeout = 5):
        start_time = time.time()
        timeout_value = timeout
        while self.is_connected() and time.time() - start_time < timeout_value:
            time.sleep(0.1)
        if time.time() - start_time > timeout_value:
            logging.error("disconnect failed")
            return -1
        return 0

    def wait_until_services_resolved(self, timeout = 5):
        start_time = time.time()
        timeout_value = timeout
        while not self.are_services_resolved and time.time() - start_time < timeout_value:
            time.sleep(0.1)
        if time.time() - start_time > timeout_value:
            logging.error("services resolution failed")
            return -1
        return 0

    """
    This call should be called from a separate thread context
    """
    def _reconnect(self):
        time.sleep(self.reconnect_time)
        logging.info("attempting reconnect")
        self._connect_to_device()

    def connect_succeeded(self):
        super().connect_succeeded()
        logging.debug("[%s] Connected" % (self.mac_address))

    def connect_failed(self, error):
        super().connect_failed(error)
        logging.debug("[%s] Connection failed: %s" % (self.mac_address, str(error)))
        if self.reconnect_attempts > 0:
            logging.info("reconnect shall be attempted in %d seconds" % self.reconnect_time)
            reconnect_thread = threading.Thread(target=self._reconnect)
            reconnect_thread.start()

    def disconnect_succeeded(self):
        super().disconnect_succeeded()
        logging.debug("[%s] Disconnected" % (self.mac_address))

    def services_resolved(self):
        self.are_services_resolved = True
        super().services_resolved()
        logging.debug("[%s] Resolved services" % (self.mac_address))
        for service in self.services:
            logging.debug("[%s]  Service [%s]" % (self.mac_address, service.uuid))
            for characteristic in service.characteristics:
                logging.debug("[%s]    Characteristic [%s]" % (self.mac_address, characteristic.uuid))


class DictofunDeviceManager(gatt.DeviceManager):
    class ConnectionStatus(Enum):
        IDLE = 1
        DISCOVERED = 2
    
    connection_status = ConnectionStatus.IDLE
    last_discovery_timestamp = 0
    timeout_discovery_duration = 5
    device = None
    dictofun = None

    def device_discovered(self, device):
        if "dict" in device.alias():
            self.last_discovery_timestamp = time.time()
            if self.connection_status == self.ConnectionStatus.IDLE:
                logging.info(device.alias() + " " + str(device.mac_address))
                self.connection_status = self.ConnectionStatus.DISCOVERED
                self.device = device

        if not self.device is None and not self.device.is_connected():
            if self.connection_status != self.ConnectionStatus.IDLE and (time.time() - self.last_discovery_timestamp > self.timeout_discovery_duration):
                logging.info("previously discovered dictofun is gone")
                self.connection_status = self.ConnectionStatus.IDLE

    # TODO: extend this getter with specified MAC address
    def get_dictofun(self):
        if self.connection_status != self.ConnectionStatus.IDLE:
            self.dictofun = DictofunBle.from_parent(self.device)
            return self.dictofun
        else:
            return None

    def wait_until_discovered(self, timeout = 5):
        start_time = time.time()
        timeout_value = timeout
        while not self.connection_status == self.ConnectionStatus.DISCOVERED and time.time() - start_time < timeout_value:
            time.sleep(0.1)
        if time.time() - start_time > timeout_value:
            logging.error("dictofun discovery has failed")
            return -1
        return 0