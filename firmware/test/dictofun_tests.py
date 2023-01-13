import serial
import time
import re
import logging
import sys

port_identifier = '/dev/ttyUSB0'

"""
This class contains set of tests that check basic dictofun functions using console.
"""
class DictofunTester:
    def __init__(self):
        self.ser = serial.Serial(port_identifier, timeout=1)
        self.ser.baudrate = 115200

    def strip_ansi_codes(self, s):
        return re.sub(r'\x1b\[([0-9,A-Z]{1,2}(;[0-9]{1,2})?(;[0-9]{3})?)?[m|K]?', '', s)
        
    def start(self):
        # reset previously issued commands
        self.issue_command("\n", 0)

    def stop(self):
        self.ser.close()

    def issue_command(self, command, wait_time = 0):
        if not command.endswith("\n"):
            command += "\n"
        self.ser.write(bytes(command, "utf-8"))
        time.sleep(0.1)
        # readback the command itself
        self.ser.readline()
        time.sleep(wait_time)
        response = ""
        line = self.ser.readline().decode("utf-8")
        while len(line) > 1:
            response += line
            line = self.ser.readline().decode("utf-8")
        
        return self.strip_ansi_codes(response)

    def reset_device(self):
        response = self.issue_command("reset", 1)
        # if there are any errors in the output: it's an issue and we have to react
        if "error" in response:
            logging.error("reset response contains word `error: %s`" % response)
            return -1

        if len(response) < 10:
            logging.error("reset response is very short: %s\n. It's very likely that something is wrong." % response)
            return -1

        if "EF-60-18" not in response:
            logging.error("Flash memory ID not found in the response: %s" % response)
            return -1

        return 0

    def check_version(self):
        command = "version"
        response = self.issue_command(command, 0)
        if not "dictofun" in response:
            logging.error("version check has failed. Received response: %s" % response)
            return -1
        return 0
            
def configure_log():
    #timestr = time.strftime("%Y%m%d-%H%M%S")
    #log_filename = "integration_test_" + timestr + ".log"
    log_filename = "debug.log"
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s [%(levelname)s] %(message)s",
        handlers=[
            logging.FileHandler(log_filename),
            logging.StreamHandler(sys.stdout)
        ]
    )

if __name__ == '__main__':
    configure_log()

    logging.info("launching dictofun tests")
    tester = DictofunTester()

    tester.start()

    if tester.reset_device() != 0:
        tester.stop()    

    tester.stop()
