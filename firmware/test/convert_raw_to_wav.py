import struct
import ctypes

def apply_wav_header(raw_data):
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

    result += struct.pack("H", 1) # type of format, 2 bytes
    result += struct.pack("H", 1) # number of channels, 2 bytes
    result += struct.pack("I", sample_rate) # sample rate
    result += struct.pack("I", byte_rate) # byte rate
    result += struct.pack("H", 2) # channel type (mono/stereo/8-16 bit)
    result += struct.pack("H", bits_per_sample) #bits per sample
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
    result = apply_wav_header(raw_data[64:])
    print(len(result))
    with open("last_record.wav", "wb") as wav:
        wav.write(result)

