
#include<thread>
#include <iostream>
#include "google_speech_2_text.h"
#include <grpcpp/grpcpp.h>
#include<chrono>

namespace speech = ::google::cloud::speech_v1;
using RecognizeStream = ::google::cloud::AsyncStreamingReadWriteRpc<
    google::cloud::speech::v1::StreamingRecognizeRequest,
    google::cloud::speech::v1::StreamingRecognizeResponse>;
    

void SpeechToTextStream::start(){
    google::cloud::speech::v1::StreamingRecognizeRequest request;
    auto& streaming_config = *request.mutable_streaming_config();
    streaming_config.mutable_config()->set_encoding(google::cloud::speech::v1::RecognitionConfig::LINEAR16);
    streaming_config.mutable_config()->set_sample_rate_hertz(44100);
    streaming_config.mutable_config()->set_language_code("en-IN");
    streaming_config.mutable_config()->set_audio_channel_count(1);
    stream = client.AsyncStreamingRecognize();

    if(!stream->Start().get()){
        std::cout<<"Unable to create stream"<<std::endl;
    }
    if(!stream->Write(request, grpc::WriteOptions{}).get()){
        std::cout<<"Unable to write config"<<std::endl;
    }
}

void SpeechToTextStream::write(void *data, size_t n_bytes){
    google::cloud::speech::v1::StreamingRecognizeRequest request;
    request.set_audio_content(data, n_bytes);
    if(!stream->Write(request, grpc::WriteOptions()).get()){
        std::cout<<"Unable to write to stream"<<std::endl;
    }else{
        std::cout<<"Wrote to stream"<<std::endl;
    }
}

void SpeechToTextStream::finish(){
    stream->WritesDone().get();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    auto read = [this] { return this->stream->Read().get(); };
    for (auto response = read(); response.has_value(); response = read()) {
        for (auto const& result : response->results()) {
            std::cout << "Result stability: " << result.stability() << "\n";
            for (auto const& alternative : result.alternatives()) {
                std::cout << alternative.confidence() << "\t"
                        << alternative.transcript() << "\n";
            }
        }
    }

    auto status = stream -> Finish().get();
    if(!status.ok()) throw status;
}
