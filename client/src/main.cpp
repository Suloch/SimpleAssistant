#include<iostream>
#include<portaudio.h>
#include<vosk_api.h>
#include<cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <csignal>
#include <stdexcept>
#include <thread>
#include <vector>
#include "logger.h"

#define SAMPLE_RATE  (44100)
#define FRAMES_PER_BUFFER (1024)
#define NUM_SECONDS (5)
#define SAMPLE_SILENCE  (0)

typedef short SAMPLE;

typedef enum{
    DATA_STREAM_START,
    DATA_STREAM_STOP,
    DATA_STREAM_CONTINUE,
}SEND_OPTION;


typedef struct {
    size_t id;
    SEND_OPTION option;
    SAMPLE *data;
    char * message;
    size_t length;
    size_t message_length;
}DataFormat;

typedef enum{
    ACK_0, // acknowledgements
    ACK_1, 
    PLAYER_STREAM,
    REPLY
}ResponseType;

typedef struct {
    ResponseType type;
    size_t length;
    void *data;
} ResponseData;

class AudioPlayer{
    public:
        std::vector<ResponseData> buffer;
        PaStream*           stream;
        PaError             err = paNoError;

        AudioPlayer(){
            
            this->start();
            
        }
        void start();
        void add_buffer(ResponseData data){
            buffer.push_back(data);
        }

        void remove_buffer(){
            //pop the data
            //delete the data.data
        }

        ~AudioPlayer(){
            err = Pa_CloseStream( stream );
            if( err != paNoError ){
                Logger::getInstance().log("Stream is not closed");
                return;
            }
        }


};
static int playCallback( const void *inputBuffer, void *outputBuffer,
                            unsigned long framesPerBuffer,
                            const PaStreamCallbackTimeInfo* timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData )
{
    AudioPlayer *data = (AudioPlayer*)userData;
    SAMPLE *rptr = (SAMPLE *)data->buffer[0].data;
    SAMPLE *wptr = (SAMPLE*)outputBuffer;
    unsigned int i;
    int finished;
    static size_t frame = 0;
    unsigned int framesLeft = data->buffer[0].length;

    (void) inputBuffer; /* Prevent unused variable warnings. */
    (void) timeInfo;
    (void) statusFlags;
    (void) userData;

    if( framesLeft < framesPerBuffer )
    {
        
        /* final buffer... */
        for( i=0; i<framesLeft; i++ )
        {
            *wptr++ = *rptr++; 
        }
        // delete the first one buffer
        data->remove_buffer();
        // update the rptr
        if(data->buffer.size() > 0){
            rptr = (SAMPLE *)data->buffer[0].data;
            /* final buffer... */
            for( ; i<framesLeft; i++ )
            {
                *wptr++ = *rptr++; 
            }
            data->buffer[0].length -= framesLeft;
            return paContinue;
        }else{
        for( ; i<framesPerBuffer; i++ )
        {
            *wptr++ = 0;  /* left */
        }
        }
        data->buffer[0].length -= framesPerBuffer;
        finished = paComplete;
    }
    else
    {
        for( i=0; i<framesPerBuffer; i++ )
        {
            *wptr++ = *rptr++;  /* left */
        }
        data->buffer[0].length -= framesPerBuffer;
        finished = paContinue;
    }
    return finished;
}

void AudioPlayer::start(){
    PaStreamParameters outputParameters;

            outputParameters.device = Pa_GetDefaultOutputDevice(); /* default output device */
            if (outputParameters.device == paNoDevice) {
                Logger::getInstance().log("Unable to get device");
                return;
            }
            outputParameters.channelCount = 1;                     /* stereo output */
            outputParameters.sampleFormat =  paInt16;
            outputParameters.suggestedLatency = Pa_GetDeviceInfo( outputParameters.device )->defaultLowOutputLatency;
            outputParameters.hostApiSpecificStreamInfo = NULL;
    err = Pa_OpenStream(
                        &stream,
                        NULL, /* no input */
                        &outputParameters,
                        SAMPLE_RATE,
                        FRAMES_PER_BUFFER,
                        paClipOff,      /* we won't output out of range samples so don't bother clipping them */
                        playCallback,
                        this );
            if( err != paNoError ){
                Logger::getInstance().log("Unable to get device");
                return;
            }
        
            if( stream )
            {
                err = Pa_StartStream( stream );
                if( err != paNoError ){
                    Logger::getInstance().log("Unable to start stream");
                    return;
                }
                
        
                while( ( err = Pa_IsStreamActive( stream ) ) == 1 ) Pa_Sleep(100);
                if( err != paNoError ){
                    Logger::getInstance().log("Stream is not active");
                    return;
                }
                
                
                
                printf("Done.\n"); fflush(stdout);
            }
}



AudioPlayer replier;



class Connection{
    public:
        bool active = true;
        int clientSocket;
        
        Connection(){
            this->clientSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (this->clientSocket < 0) {
        return;
    }

            // Server address and port
            sockaddr_in serverAddr;
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(8080); // Port the server is listening on
            serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // IP address of the server

            // Connect to server
            if (connect(this->clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
                return;
            }



        }

        std::vector<char> serialize(DataFormat data, size_t len){
            std::vector<char> req_data(len);

            memcpy(req_data.data(), &data, sizeof(DataFormat));

            memcpy(req_data.data()+sizeof(DataFormat), data.data, data.length*sizeof(SAMPLE));
            memcpy(req_data.data()+sizeof(DataFormat)+sizeof(SAMPLE)*data.length, data.message, data.message_length);

            return req_data;
        }

        void sendData(const SAMPLE *in, size_t len, size_t id, SEND_OPTION option){
            SAMPLE sample_data[len];
            memcpy(sample_data, in, len);

            char message[] = "Sending voice data";

            DataFormat data{id, option, sample_data, message, len, strlen(message)};

            size_t data_len = sizeof(DataFormat) + sizeof(char) * strlen(message) + sizeof(SAMPLE) * len + 1;

            std::vector<char> req_data = serialize(data, data_len);
            if (send(this->clientSocket, req_data.data(), data_len, 0) < 0) {
                        std::cerr << "Error sending data\n";
            }

        }
        ResponseData deserialize_response(char *buffer){
            ResponseData data;
            memcpy(&data, buffer, sizeof(ResponseData));
            if(data.type == REPLY){
                data.data = new SAMPLE[data.length];
                memcpy(data.data, buffer + sizeof(ResponseData), sizeof(SAMPLE) * data.length); 
                replier.add_buffer(data);
            }

            return data;
        }

        void recieveData(){
            char message[65*1024];
            while(active){
                if(recv(clientSocket, message, 65*1024, 0)){
                    std::cout<<message<<std::endl;
                    ResponseData data = deserialize_response(message);
                }
            }
        }

        ~Connection(){
            close(clientSocket);
        }
};

Connection C;
const unsigned long frames_to_send = NUM_SECONDS * SAMPLE_RATE;

// std::thread listener_thread(&Connection::recieveData, C);

static int listenerCallback(
            const void* input_buffer, 
            void *output_buffer,
            unsigned long frames_per_buffer,
            const PaStreamCallbackTimeInfo *time_info,
            PaStreamCallbackFlags status_flags,
            void *user_data
            ){
    VoskRecognizer *recognizer = (VoskRecognizer *)user_data;
    const SAMPLE * in = (const SAMPLE *)input_buffer;

    static bool sending = false;
    static size_t id = 0;
    static size_t sent_frames = 0;


    //if sending data skip recognition
    if(sending){
        //continue sending
        sent_frames += frames_per_buffer;

        //check if this is the last frame
        if(sent_frames > frames_to_send){
            sending = false;
            C.sendData(in, frames_per_buffer, id, DATA_STREAM_STOP);
            id = 0;
            return paContinue;

        }
        C.sendData(in, frames_per_buffer, id, DATA_STREAM_CONTINUE);
        id++;
        return paContinue;
    }

    int final = vosk_recognizer_accept_waveform_s(recognizer, in, frames_per_buffer);
    if(final){
        const char *s = vosk_recognizer_result(recognizer);
        std::cout<<s<<std::endl;
        if(strstr(s,"night") or strstr(s, "assistant")){
            //start sending data

            sending = true;
            C.sendData(in, frames_per_buffer, id, DATA_STREAM_START);
            id++;
        }
    }

    return paContinue;
}



class NameListener{

    public:
        VoskRecognizer * recognizer;
        NameListener(VoskRecognizer * recognizer){
            this->recognizer = recognizer;
            this->initPortAudio();
        }

        ~NameListener(){
            err = Pa_StopStream(stream);
            err = Pa_CloseStream(stream);
        }
    private:

        PaStream *stream;
        PaError err;
        PaStreamParameters input_parameters;

        void initPortAudio(){
            err = Pa_Initialize();

            input_parameters.device = 11;
            std::cout<<input_parameters.device<<std::endl;
            input_parameters.channelCount = 1; 
            input_parameters.sampleFormat = paInt16; 
            input_parameters.suggestedLatency = Pa_GetDeviceInfo(input_parameters.device)->defaultLowInputLatency;
            input_parameters.hostApiSpecificStreamInfo = NULL;

            err = Pa_OpenStream(&stream,
                        &input_parameters, // Specify input parameters
                        NULL,      // No output
                        SAMPLE_RATE,
                        FRAMES_PER_BUFFER,
                        paClipOff, // Turn off clipping
                        listenerCallback,
                        recognizer);
            if (err != paNoError) showError("Error while opening stream");

            err = Pa_StartStream(stream);
            if (err!= paNoError) showError("Error while starting stream");
        }

        void showError(const char *msg){
            std::cout<<msg<<std::endl;
        }

        
};



int main(int argc, char** argv){
    Logger::getInstance().start(debug, "log.txt");
    VoskModel *model = vosk_model_new("../../models/model2");
    VoskRecognizer *recognizer= vosk_recognizer_new(model, SAMPLE_RATE);

    Logger::getInstance().log("Vosk initialized");
    
    NameListener *app = new NameListener(recognizer);
    Logger::getInstance().log("Listenre initialzied");
    
    std::cout<<"Press Enter to exit"<<std::endl;
    getchar();
    C.active = false;
    delete app;
    vosk_recognizer_free(recognizer);
    vosk_model_free(model);
    Logger::getInstance().stop();

    return 0;

}









