import wave
import logging
import sys

if __name__ == '__main__':
    if len(sys.argv) != 3:
        logging.error("Wrong script syntax. Should have an input and output file names")
        exit(-1)
    input_name = sys.argv[1]
    output_name = sys.argv[2]

    raw_data = bytearray([])
    if not output_name.endswith(".wav"):
        output_name += ".wav"

    try:
        with open(input_name, "rb") as raw:
            byte = raw.read(1)
            raw_data += byte
            while byte:
                byte = raw.read(1)
                raw_data += byte
    except Exception as e:
        logging.error("failed to open file " + input_name + ": " + str(e))
        sys.exit(-1)

    try:
        with wave.open(output_name, "wb") as record:
            record.setnchannels(1)
            record.setsampwidth(2)
            record.setframerate(8000)
            record.writeframes(raw_data[0x100:])
    except Exception as e:
        logging.error("failed to create record file " + output_name + ": " + str(e))
