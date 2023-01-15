import serial
import time
import re

class DictofunControl:
    def __init__(self, port):
        self.ser = serial.Serial(port, timeout=1)
        self.ser.baudrate = 115200
        # send a single `\n` to reset previous commands 
        self.issue_command("\n")

    def issue_command(self, command, wait_time = 0):
        if not command.endswith("\n"):
            command += "\n"
        self.ser.write(bytes(command, "utf-8"))
        time.sleep(0.05)
        # readback the command itself
        self.ser.readline()
        time.sleep(wait_time)
        response = ""
        line = self.ser.readline().decode("utf-8")
        while len(line) > 1:
            response += line
            line = self.ser.readline().decode("utf-8")

        return self.strip_ansi_codes(response)

    def strip_ansi_codes(self, s):
        return re.sub(r'\x1b\[([0-9,A-Z]{1,2}(;[0-9]{1,2})?(;[0-9]{3})?)?[m|K]?', '', s)
