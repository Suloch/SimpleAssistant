#pragma once
#include "google/cloud/speech/v1/speech_client.h"
#include "google/cloud/project.h"

namespace speech = ::google::cloud::speech_v1;
using RecognizeStream = ::google::cloud::AsyncStreamingReadWriteRpc<
    google::cloud::speech::v1::StreamingRecognizeRequest,
    google::cloud::speech::v1::StreamingRecognizeResponse>;
    

class SpeechToTextStream{
    public:
        speech::SpeechClient client = speech::SpeechClient(speech::MakeSpeechConnection());
        std::unique_ptr<RecognizeStream> stream;

        void start();

        void write(void *data, size_t n_bytes);

        void finish();

};