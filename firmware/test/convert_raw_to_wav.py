import wave
import logging
import sys
import struct
from adpcm import adpcm_decode

wav_format_codes = {
    "raw" : 0x01,
    "adpcm" : 0x11
}

def decode_file_type(raw):
    descriptor = {}
    if raw.startswith("adpcm".encode("utf-8")):
        descriptor["type"] = "adpcm"
        
        frequency_start_pos = raw.find(";".encode("utf-8"))
        frequency_end_pos = frequency_start_pos + raw[frequency_start_pos+1:].find(";".encode("utf-8"))
        frequency = int(raw[frequency_start_pos+1:frequency_end_pos+1].decode("utf-8"))
        descriptor["frequency"] = frequency

        sample_width = int(raw[frequency_end_pos+2:frequency_end_pos+3])
        descriptor["sample_width"] = sample_width

        # wipe out content of the header for the further conversion
        for i in range(0, frequency_end_pos+3):
            raw[i] = 0
    elif raw.startswith("decim".encode("utf-8")):
        descriptor["type"] = "raw"
        
        frequency_start_pos = raw.find(";".encode("utf-8"))
        frequency_end_pos = frequency_start_pos + raw[frequency_start_pos+1:].find(";".encode("utf-8"))
        
        frequency = int(raw[frequency_start_pos+1:frequency_end_pos+1].decode("utf-8"))
        descriptor["frequency"] = frequency

        sample_width = int(raw[frequency_end_pos+2:frequency_end_pos+3])
        descriptor["sample_width"] = sample_width

        decimation_factor = int(raw[frequency_end_pos+4:frequency_end_pos+5])
        descriptor["decimation_factor"] = decimation_factor

        for i in range(0, frequency_end_pos+5):
            raw[i] = 0
    
    return descriptor


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

    descriptor = decode_file_type(raw_data)

    if len(descriptor) == 0:
        logging.error("unknown descriptor. Exiting")
        sys.exit(-1)

    if descriptor["type"] == "raw":
        try:
            with wave.open(output_name, "wb") as record:
                record.setnchannels(1)
                record.setsampwidth(descriptor["sample_width"])
                record.setframerate(descriptor["frequency"] / descriptor["decimation_factor"])
                record.writeframes(raw_data[0x200:])
        except Exception as e:
            logging.error("failed to create record file " + output_name + ": " + str(e))

    elif descriptor["type"] == "adpcm":
        try:
            decoded = adpcm_decode(raw_data[0x100:])
            with wave.open(output_name, "wb") as record:
                record.setnchannels(1)
                record.setsampwidth(descriptor["sample_width"])
                record.setframerate(descriptor["frequency"])
                record.writeframes(decoded)
        except Exception as e:
            logging.error("exception during header application: " + str(e))
