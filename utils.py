import requests
import pydub
import audio_metadata
import json

def Get_MusicRecognition_Result(filename):
    data = {
        'api_token': '44c6ff8b0c48088f01c4367de84a0adf',
        'return': 'apple_music,spotify',
    }

    files = {
        'file': open(filename, 'rb')
    }

    result = requests.post('https://api.audd.io/', data=data, files=files)
    return result.text

def mp32wav(in_file, out_file):
    from pydub import AudioSegment
    sound = AudioSegment.from_mp3(in_file)
    sound.export(out_file, format="wav")

def wav2mp3(in_file, out_file):
    from pydub import AudioSegment
    sound = AudioSegment.from_wav(in_file)
    sound.export(out_file, format="mp3")

def array2wav(Byte_Array, out_file):
    wav_header = [
        82, 73, 70, 70,     # RIFF
        44, 250, 0, 0,      # FileSize <--- [4:8]
        87, 65, 86, 69,     # WAVE
        102, 109, 116, 32,  # fmt
        16, 0, 0, 0,        # length of format data
        1, 0,               # type of format (1=PCM)
        1, 0,               # number of channels
        128, 62, 0, 0,      # wavFreq <--- [24:28]
        0, 125, 0, 0,       # (Sample Rate * BitsPerSample * Channels) / 8
        2, 0, 16, 0,
        100, 97, 116, 97,   # data
        0, 250, 0, 0        # DataSize <--- [40:44]
    ]

    fileSize = len(Byte_Array) + 44
    dataSize = len(Byte_Array)
    wavFreq  = 16000

    wav_header[ 4: 8] = [fileSize&255, (fileSize>>8)&255, (fileSize>>16)&255, (fileSize>>24)&255]
    wav_header[24:28] = [wavFreq&255 , (wavFreq>>8)&255 , (wavFreq>>16)&255 , (wavFreq>>24)&255 ]
    wav_header[40:44] = [dataSize&255, (dataSize>>8)&255, (dataSize>>16)&255, (dataSize>>24)&255]

    Byte_Array = wav_header + Byte_Array

    with open(out_file, 'wb') as f:
        f.write(bytearray(Byte_Array))


