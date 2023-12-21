from utils import *
import socket
import random
import time
import json
import threading

HOST = '172.20.10.13'
PORT = 5100

checkpoint = 3*2    # (init) 3 seconds
Byte_Array = []     # Store the audio data
wavFreq    = 16000  
Music_Info = None
comm = 0            # 0: no connection, 1: connection
done = 0            # 0: processing, 1: success, 2: failure

MAX_RECOGNITION_QUOTA = 1
Quota = threading.Semaphore(MAX_RECOGNITION_QUOTA)

def receiver():
    global Byte_Array, comm, done
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((HOST, PORT))
        s.listen()
        print('Waiting for connection!')
        conn, addr = s.accept()
        with conn:
            start_time = time.time()
            duration = 0
            comm = 1
            print("Connected by ", addr)
            while not done:
                if (duration>=40):
                    # print("Runtime error!")
                    done = 2
                    break
                data = conn.recv(1024)
                data_array = list(data)
                Byte_Array += data_array
                duration = time.time()-start_time
            comm = 0
            s.close()

def recognizer():
    global checkpoint, Byte_Array, wavFreq, Music_Info, comm, done, Quota
    while not done:
        # if (len(Byte_Array) >= checkpoint*wavFreq):
        Quota.acquire()
        while comm and len(Byte_Array) >= 6*wavFreq:
            checkpoint += 2 
            array2wav(Byte_Array, 'music.wav')

            try:
                result = json.loads(Get_MusicRecognition_Result('music.wav'))
            except:
                # print("Recognition failure!")
                done = 2

            if (result['status']=='success' and result['result']!=None):
                # Music_Info = result['result']['artist'] + ' - ' + result['result']['title']
                Music_Info = f"{result['result']['artist']} - {result['result']['title']}\nAlbum: {result['result']['album']}\nRelease date: {result['result']['release_date']}\nSong link: {result['result']['song_link']}"
                # print(Music_Info)
                done = 1
            else:
                pass
                # print('Recognition status: ', result['status'], ', and keep listening')
        Quota.release()

if __name__ == "__main__":
    receiver_thread = threading.Thread(target = receiver)
    receiver_thread.start()
    recognizer()
    receiver_thread.join()
    
    print("*****************************************")
    if done == 1:
        print(Music_Info)
    elif done == 2:
        print("Recognition Failure!")
    print("*****************************************")
