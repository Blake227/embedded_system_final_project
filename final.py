from utils import *
import socket
import random
import time
import json
import threading

HOST = '172.20.10.13'
PORT = 5100

checkpoint = 3*2      # (init) 3 seconds
Byte_Array = []     # Store the audio data
wavFreq    = 16000  
Music_Info = None
done = 0

def receiver():
    global Byte_Array, done
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((HOST, PORT))
        s.listen()
        conn, addr = s.accept()
        with conn:
            print("Connected by ", addr)
            while not done:
                data = conn.recv(1024)
                data_array = list(data)
                Byte_Array += data_array

def recognizer():
    global checkpoint, Byte_Array, wavFreq, Music_Info, done
    while not done:
        if (len(Byte_Array) >= checkpoint*wavFreq):
            checkpoint += 2
            array2wav(Byte_Array, 'music.wav')
            result = json.loads(Get_MusicRecognition_Result('music.wav'))
            if (result['status']=='success' and result['result']!=None):
                Music_Info = result['result']['artist'] + ' ' + result['result']['title']
                print(Music_Info)
                done = 1
            else: 
                print('Recognition status: ', result['status'], ', and keep listening')

if __name__ == "__main__":
    receiver_thread = threading.Thread(target = receiver)
    recognizer_thread = threading.Thread(target = recognizer)
    
    receiver_thread.start()
    recognizer_thread.start()

    receiver_thread.join()
    recognizer_thread.join()
