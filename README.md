# NTU 2023 Fall Embedded System Final Project - Music Recognition System


## Motivation
It is relatively easy to do music recognition on mobile phones. However, in the IoT devices with limited computational resources, it is difficult to save audio data for this service. Therefore, we implemented a DMA method to achieve music recognition on STM32, expecting this feature to be applicable to IoT devices like AR glasses or bluetooth headphones.


## Methodology
1. Utilizing microphone on STM32 to collect audio data
2. Using BSP callback function to store the audio data
3. Transferring audio data between two devices using Wi-Fi protocol
4. Collecting audio data on Raspberry Pi and convert data into WAV file
5. Employing AudD API to do music recognition
6. Print the result to the terminal

## Workflow

![Screenshot 2023-12-21 at 9.23.02 PM](https://hackmd.io/_uploads/rytVS3Gw6.png)

## Implementation
### STM32
#### Audio Collect
- Using DMA with callback function to collect audio data from microphone
- Buffer size: PCM_buffer = 32, TARGET_AUDIO_BUFFER = 32000
- Audio data directly loaded into PCM_buffer, then transfer to TARGET_AUDIO_BUFFER by callback function every second
![Screenshot 2023-12-22 at 3.12.36 PM](https://hackmd.io/_uploads/rk_lPnMv6.png)
<br>

* Send audio data through Wi-Fi when TARGET_AUDIO_BUFFER half full.
* If data sent successfully, then continue. Otherwise, re-send data. If re-send data more than third times, end the detection.
![Screenshot 2023-12-22 at 3.16.18 PM](https://hackmd.io/_uploads/HktCvnMD6.png)

<br>
- Music recognition usually ends up in 10 seconds, and the API can only upload 1.5Mb file, which approximately equals to 40 seconds. 
- Hence, after detecting for 40 seconds, we will end recoding sound and sending data.




### Raspberry Pi
The role of Raspberry Pi is both receiver and recognizer. It keeps receiving the audio data sent from STM32 through WiFi, and gathers the data for the API to get the recognition result.

To receive data from STM32, we first build a socket to connect both devices, and then begin to transport data. \Since AuDD API needs to take the audio file for recognition, we periodically save the audio data came from STM32 in wav format. According to our trials, the duration of an audio needs to be at least 3 seconds for the API, so it determines that our first saving would be at 3 seconds. After the saving, the file is then recognized by the API, and the system will terminate if successful recognition. Otherwise, we will continue to concatenate every 1 second audio data to the previous one until successful recognition.

#### Multi-threading
In our application, the role of Raspberry Pi is both **receiver** and **recognizer**.
As a result, we build two threads to 
* Receive the audio data from STM32 through WiFi
* Save the audio in WAV format & Send to API for recognition

#### Receiver
* Build a connection with socket
* Once two devices are connected, RPi begins to receive data and starts a timer to count down
* Continue adding data to a buffer until the recognition process is completed or the connection has been active for 40 seconds

#### Recognizer
* Start the recognition process after 		the connection is built and the duration of the audio data reaches 3 seconds
* To avoid making frequent API requests, 	use a semaphore to manage the quota of requests at any given time
* The process stops 					either upon successful recognition 		  or in the event of an error
* The recognition process includes wav format conversion and API requesting
* For data conversion to WAV format, you simply need to append the audio data directly after the header that specifies the metadata of the audio
* To utilize the Audd API, you only need to submit a request with the audio file, and you will receive the recognition results

## Result
* Succeessful Recognition: https://drive.google.com/file/d/1QZrCX5aGHPpW3ftMoYBY-pw2HVJzKS0k/view?usp=share_link
* Process takes too long: https://drive.google.com/file/d/1aAket15TI1ZkinOYBrlGg5uXZAw03eDT/view?usp=share_link

## Difficulties we encountered and Future Works
### Denoise
We tried to implemented the functionality of FIR to filter out the high frequency of noise. Nevertheless, when we used the Fir library, we found out that based on the 16k sampling rate, we can only filter out the noise with frequency no more than 8k, which would distort the music, failing the recognition.

Moreover, we also faced the problem that the speed of the filter is slower than the input rate, leading to data overlap and making audio become noise.

To address the problem, we might need to use multi-thread on STM32 too. Additionally, we might need to raise the sampling rate to increase the cutoff frequency.


### Continuous Recognition
In the future, this application could be extended to a continuous version, which can do music recognition multiple times with starting this program once. To achieve this feature, we need to add a function that could receive finish signal from Raspberry Pi on STM32, and reset the memory on both STM32 and Raspberry Pi.


###
## Reference
* Audio recording
    * BSP library: https://reurl.cc/OG1D1g
    * mbed microphone project: https://reurl.cc/13xz4p
* WiFi Mbed Project: 
https://github.com/ARMmbed/mbed-os-example-sockets
* Waveform Audio File Format: 
https://docs.fileformat.com/audio/wav/
* Audd API Documentation: https://docs.audd.io/
* Threading Library (Thread, Semaphore) https://docs.python.org/3/library/threading.html




## How to use the system?

To utilize the system, you have to register an account on **Audd.io** to acquire an API token. Your API token can be found in Audd's dashboard, and enter it to the function **Get_MusicRecognition_Result** in **utils.py**. 

In order to connect both STM32 and RPi, you need to modify the IP address and port in **final.py**. 

Note that RPi needs to be accessible to Internet since it has to request the API platform.

Execute the following command to start music recognition.
```
python final.py
```

