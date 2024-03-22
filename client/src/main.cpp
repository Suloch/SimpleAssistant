#include<iostream>
#include<portaudio.h>
#include<vosk_api.h>
#include<cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <csignal>

#define SAMPLE_RATE  (44100)
#define FRAMES_PER_BUFFER (1024)
#define NUM_SECONDS (5)
#define SAMPLE_SILENCE  (0)

typedef short SAMPLE;

typedef struct {
    unsigned int frame_index;
    unsigned int max_frame_index;
    bool is_recording;
    VoskRecognizer *recognizer;
    SAMPLE *samples;
}VoiceData;

class Connection{
    public:
        int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
        Connection(){
            if (clientSocket < 0) {
                std::cerr << "Error creating socket\n";
            }

            // Server address and port
            sockaddr_in serverAddr;
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(8080); // Port the server is listening on
            serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // IP address of the server

            // Connect to server
            if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
                std::cerr << "Connection failed\n";
            }
        }
        
        int sendData(const char* message){

            // Send data
            if (send(clientSocket, message, strlen(message), 0) < 0) {
                std::cerr << "Error sending data\n";
                return 1;
            }
            return 0;
        }

        ~Connection(){
            close(clientSocket);
        }
};


static int listenerCallback(
            const void* input_buffer, 
            void *output_buffer,
            unsigned long frames_per_buffer,
            const PaStreamCallbackTimeInfo *time_info,
            PaStreamCallbackFlags status_flags,
            void *user_data
            ){
    VoiceData *data = (VoiceData *)user_data;
    const SAMPLE * in = (const SAMPLE *)input_buffer;

    int final = vosk_recognizer_accept_waveform_s(data->recognizer, in, frames_per_buffer);
    if(final){
        const char *s = vosk_recognizer_result(data->recognizer);
        std::cout<<s<<std::endl;
        if(strstr(s,"night") or strstr(s, "assistant")){
            std::cout<<"HERE"<<std::endl;
        }
    }
    return paContinue;
}



class NameListener{

    public:
        VoiceData data;
        NameListener(VoskRecognizer * recognizer){
            this->initVoiceData(recognizer);
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

        
        void initVoiceData(VoskRecognizer *recognizer){
            data.max_frame_index = NUM_SECONDS * SAMPLE_RATE;
            data.frame_index = 0;
            data.samples = (SAMPLE *) malloc( data.max_frame_index * sizeof(SAMPLE) );
            data.is_recording = false;
            data.recognizer = recognizer;
        }

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
                        &data);
            if (err != paNoError) showError("Error while opening stream");

            err = Pa_StartStream(stream);
            if (err!= paNoError) showError("Error while starting stream");
        }

        void showError(const char *msg){
            std::cout<<msg<<std::endl;
        }

        
};



int main(int argc, char** argv){

    VoskModel *model = vosk_model_new("../../models/model2");
    VoskRecognizer *recognizer= vosk_recognizer_new(model, SAMPLE_RATE);
    
    NameListener *app = new NameListener(recognizer);
    
    std::cout<<"Press Enter to exit"<<std::endl;
    
    delete app;
    vosk_recognizer_free(recognizer);
    vosk_model_free(model);
    return 0;

}









