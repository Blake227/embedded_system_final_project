from utils import *
import socket
import random
import time
import json

HOST = '172.20.10.13'
PORT = 5100

checkpoint = 3*2      # (init) 3 seconds
Byte_Array = []     # Store the audio data
wavFreq    = 16000  
Music_Info = None

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind((HOST, PORT))
    s.listen()
    conn, addr = s.accept()
    with conn:
        print("Connected by ", addr)
        while True:
            data = conn.recv(1024)
            data_array = list(data)
            Byte_Array += data_array
            if (len(Byte_Array) >= checkpoint*wavFreq):
                array2wav(Byte_Array, 'music.wav')
                result = json.loads(Get_MusicRecognition_Result('music.wav'))
                if (result['status']=='success' and result['result']!=None):
                    Music_Info = result['result']['artist'] + ' ' + result['result']['title']
                    print(Music_Info)
                    break
                else: 
                    print('Recognition status: ', result['status'], ', and keep listening')
                checkpoint += 2

