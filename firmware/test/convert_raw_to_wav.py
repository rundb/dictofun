import struct

def apply_wav_header(raw_data):
    result = bytearray([])
    result += "RIFF".encode('utf-8')
    file_size = len(raw_data) + 36
    result += struct.pack("I", file_size)
    result += "WAVE".encode('utf-8')
    result += "fmt\0".encode('utf-8')
    result += struct.pack("I", 16)
    result += struct.pack("H", 1) # type of format, 2 bytes
    result += struct.pack("H", 1) # number of channels, 2 bytes
    sample_rate = 16000
    result += struct.pack("I", sample_rate) # sample rate
    next_value = int(sample_rate * 8 * 1 / 8)
    result += struct.pack("I", next_value)
    result += struct.pack("H", 1)
    result += struct.pack("H", 8) #bits per sample
    result += "data".encode('utf-8')
    result += struct.pack("I", len(raw_data))

    result += raw_data
    return result

if __name__ == '__main__':
    raw_data = bytearray([])

    with open("last_record.bin", "rb") as raw:
        byte = raw.read(1)
        raw_data += byte
        while byte:
            byte = raw.read(1)
            raw_data += byte
    print(len(raw_data))
    result = apply_wav_header(raw_data)
    print(len(result))
    with open("last_record.wav", "wb") as wav:
        wav.write(result)
