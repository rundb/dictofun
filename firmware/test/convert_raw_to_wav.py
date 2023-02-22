import wave
import logging
import sys
import struct

wav_format_codes = {
    "raw" : 1,
    "adpcm" : 0x11
}

def apply_wav_header(raw_data, format):
    result = bytearray([])
    file_size = len(raw_data) + 36
    sample_rate = 16000
    bits_per_sample = 16
    byte_rate = int(sample_rate * bits_per_sample * 1 / 8)

    result += "RIFF".encode('utf-8') # 1 - 4
    result += struct.pack("I", file_size) # File size
    result += "WAVE".encode('utf-8') 
    result += "fmt ".encode('utf-8')
    result += struct.pack("I", 16)

    result += struct.pack("H", wav_format_codes[format]) # type of format
    result += struct.pack("H", 1) # number of channels, 2 bytes
    result += struct.pack("I", sample_rate) # sample rate
    result += struct.pack("I", byte_rate) # byte rate
    result += struct.pack("H", 2) # channel type (mono/stereo/8-16 bit)
    result += struct.pack("H", bits_per_sample) #bits per sample
    result += "data".encode('utf-8')
    result += struct.pack("I", len(raw_data))

    result += raw_data
    return result

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

def decode_adpcm_record(raw):
    result = bytearray([])

    return result
        

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

    print("file descriptor: " + str(descriptor))

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
            output = apply_wav_header(raw_data, descriptor["type"])
            with open(output_name, "wb") as f:
                f.write(output)
        except Exception as e:
            logging.error("exception during header application: " + str(e))

        