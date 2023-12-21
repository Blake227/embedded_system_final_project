/* Sockets Example
 * Copyright (c) 2016-2020 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "mbed.h"
#include "ThisThread.h"
#include "wifi.h"
#include "stm32l475e_iot01_audio.h"
#include <cstdint>
#include <cstdio>


static float32_t PCM_Buffer[PCM_BUFFER_LEN / 2];
static BSP_AUDIO_Init_t MicParams;
static EventQueue ev_queue;
InterruptIn btn(BUTTON1);
FIR_f32<NUM_TAPS> fir(firCoeffs32);
SocketDemo socketdemo;

// Place to store final audio (alloc on the heap), here two seconds...
static size_t TARGET_AUDIO_BUFFER_NB_SAMPLES = AUDIO_SAMPLING_FREQUENCY * 2;
static int16_t *TARGET_AUDIO_BUFFER = (int16_t*)calloc(TARGET_AUDIO_BUFFER_NB_SAMPLES, sizeof(int16_t));
static size_t TARGET_AUDIO_BUFFER_IX = 0;

// we skip the first 50 events (100 ms.) to not record the button click
static size_t SKIP_FIRST_EVENTS = 50;
static size_t half_transfer_events = 0;
static size_t transfer_complete_events = 0;
int recordtime = 0;
bool full = true, successSent = false, detected = false;


void start_recording() {
    int32_t ret;
    uint32_t state;

    ret = BSP_AUDIO_IN_GetState(AUDIO_INSTANCE, &state);
    if (ret != BSP_ERROR_NONE) {
        printf("Cannot start recording: Error getting audio state (%d)\n", ret);
        return;
    }
    if (state == AUDIO_IN_STATE_RECORDING) {
        printf("Cannot start recording: Already recording\n");
        return;
    }

    // reset audio buffer location
    TARGET_AUDIO_BUFFER_IX = 0;
    transfer_complete_events = 0;
    half_transfer_events = 0;

    ret = BSP_AUDIO_IN_Record(AUDIO_INSTANCE, (uint8_t *) PCM_Buffer, PCM_BUFFER_LEN);
    if (ret != BSP_ERROR_NONE) {
        printf("Error Audio Record (%ld)\n", ret);
        return;
    }
    else {
        printf("OK Audio Record\n");
    }
}

void restart(){
    recordtime = 0;
    full = true;
    successSent = false;
    printf("Press the BLUE button to record a message\n");
    btn.fall(ev_queue.event(&start_recording));
}

// callback that gets invoked when TARGET_AUDIO_BUFFER is full
void target_audio_buffer_full() {
    // pause audio stream
    printf("%d second ~ %d second recorded.\n", recordtime, recordtime + 2);
    
    recordtime += 2;
    if (recordtime >= 60){
        BSP_AUDIO_IN_Stop(AUDIO_INSTANCE);
        printf("Detection over time.\n");
        ev_queue.call(restart);
    }
}

/**
* @brief  Half Transfer user callback, called by BSP functions.
* @param  None
* @retval None
*/
void sendHalf(){
    int counter = 0;
    do{
        counter++;
        successSent = socketdemo.send_data(TARGET_AUDIO_BUFFER, 32000);
        if (counter == 3){
            BSP_AUDIO_IN_Stop(AUDIO_INSTANCE);
            printf("Drop connection.\n");
            ev_queue.call(restart);
            break;
        }
        counter++;
    }while (!successSent);
}
void BSP_AUDIO_IN_HalfTransfer_CallBack(uint32_t Instance) {
    half_transfer_events++;
    if (half_transfer_events < SKIP_FIRST_EVENTS) return;
    uint32_t buffer_size = PCM_BUFFER_LEN / 2; /* Half Transfer */
    uint32_t nb_samples = buffer_size / sizeof(int16_t); /* Bytes to Length */
    
    /* Copy first half of PCM_Buffer from Microphones onto Fill_Buffer */
    memcpy(((uint8_t*)TARGET_AUDIO_BUFFER) + (TARGET_AUDIO_BUFFER_IX * 2), PCM_Buffer, buffer_size);
    
    TARGET_AUDIO_BUFFER_IX += nb_samples;

    if (TARGET_AUDIO_BUFFER_IX >= TARGET_AUDIO_BUFFER_NB_SAMPLES / 2 && full == true) {
        ev_queue.call(sendHalf);
        full = false;
    }
}

/**
* @brief  Transfer Complete user callback, called by BSP functions.
* @param  None
* @retval None
*/
void sendSecondHalf(){
    int counter = 0;
    do{
        successSent = socketdemo.send_data(TARGET_AUDIO_BUFFER + 16000, 32000);
        if (counter == 3){
            BSP_AUDIO_IN_Stop(AUDIO_INSTANCE);
            printf("Drop connection.\n");
            ev_queue.call(restart);
            break;
        }
        counter++;
    }while (!successSent);
}
void BSP_AUDIO_IN_TransferComplete_CallBack(uint32_t Instance) {
    transfer_complete_events++;
    if (transfer_complete_events < SKIP_FIRST_EVENTS) return;
    uint32_t buffer_size = PCM_BUFFER_LEN / 2; /* Half Transfer */
    uint32_t nb_samples = buffer_size / sizeof(int16_t); /* Bytes to Length */

    /* Copy second half of PCM_Buffer from Microphones onto Fill_Buffer */
    memcpy(((uint8_t*)TARGET_AUDIO_BUFFER) + (TARGET_AUDIO_BUFFER_IX * 2), ((uint8_t*)PCM_Buffer) + (nb_samples * 2), buffer_size);

    TARGET_AUDIO_BUFFER_IX += nb_samples;
    if (TARGET_AUDIO_BUFFER_IX >= TARGET_AUDIO_BUFFER_NB_SAMPLES && full == false) {
        ev_queue.call(sendSecondHalf);
        ev_queue.call(target_audio_buffer_full);
        full = true;
        TARGET_AUDIO_BUFFER_IX = 0;
    }
}

/**
  * @brief  Manages the BSP audio in error event.
  * @param  Instance Audio in instance.
  * @retval None.
  */
void BSP_AUDIO_IN_Error_CallBack(uint32_t Instance) {
    BSP_AUDIO_IN_Stop(Instance);
    printf("BSP_AUDIO_IN_Error_CallBack\n");
}

void print_stats() {
}


int main() {
#ifdef MBED_CONF_MBED_TRACE_ENABLE
    mbed_trace_init();
#endif

    MBED_ASSERT(&socketdemo);
    socketdemo.run();

    if (!TARGET_AUDIO_BUFFER) {
        printf("Failed to allocate TARGET_AUDIO_BUFFER buffer\n");
        return 0;
    }

    // set up the microphone
    MicParams.BitsPerSample = 16;
    MicParams.ChannelsNbr = AUDIO_CHANNELS;
    MicParams.Device = AUDIO_IN_DIGITAL_MIC1;
    MicParams.SampleRate = AUDIO_SAMPLING_FREQUENCY;
    MicParams.Volume = 32;

    int32_t ret = BSP_AUDIO_IN_Init(AUDIO_INSTANCE, &MicParams);

    if (ret != BSP_ERROR_NONE) {
        printf("Error Audio Init (%ld)\r\n", ret);
        return 1;
    } else {
        printf("OK Audio Init\t(Audio Freq=%ld)\r\n", AUDIO_SAMPLING_FREQUENCY);
    }

    printf("Press the BLUE button to record a message\n");

    // hit the blue button to record a message
    btn.fall(ev_queue.event(&start_recording));

    ev_queue.dispatch_forever();
    return 0;
}
