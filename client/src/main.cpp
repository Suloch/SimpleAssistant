#include<iostream>
#include<portaudio.h>
#include<vosk_api.h>


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
    }
    return paContinue;
}

class NameListener{

    public:
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
        VoiceData data;

        
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
    const char *name = "night";

    VoskModel *model = vosk_model_new("../../models/model2");
    VoskRecognizer *recognizer= vosk_recognizer_new(model, SAMPLE_RATE);

    NameListener app = NameListener(recognizer);
    
    std::cout<<"Press Enter to exit"<<std::endl;
    getchar();

    vosk_recognizer_free(recognizer);
    vosk_model_free(model);
    return 0;
}









