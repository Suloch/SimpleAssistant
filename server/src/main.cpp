#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <csignal>
#include <cstdlib>
#include<sstream>
#include<thread>
#include<map>
#include<chrono>
#include<mutex>

#include "google/cloud/speech/v1/speech_client.h"
#include "google/cloud/project.h"

#include <grpcpp/grpcpp.h>


namespace speech = ::google::cloud::speech_v1;
using RecognizeStream = ::google::cloud::AsyncStreamingReadWriteRpc<
    google::cloud::speech::v1::StreamingRecognizeRequest,
    google::cloud::speech::v1::StreamingRecognizeResponse>;
    

typedef short SAMPLE;
#define SAMPLE_RATE  (44100)
#define NUM_SECONDS (5)


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

const int BUFFER_SIZE = sizeof(DataFormat) + sizeof(char) *100 + SAMPLE_RATE * NUM_SECONDS * sizeof(SAMPLE);
std::mutex data_buffer_mutex;

using namespace google::cloud::speech::v1;
class SpeechRecognitionStream{
    
    private:
    
    std::unique_ptr<RecognizeStream> stream;
    bool running = false;

    public:

    void startStream(){
        if(running){
            std::cout<<"Stream Already running"<<std::endl;
            return;
        }

        auto client = speech::SpeechClient(speech::MakeSpeechConnection());

        google::cloud::speech::v1::StreamingRecognizeRequest request;
        auto& streaming_config = *request.mutable_streaming_config();
        streaming_config.mutable_config()->set_encoding(RecognitionConfig::LINEAR16);
        streaming_config.mutable_config()->set_sample_rate_hertz(44100);
        streaming_config.mutable_config()->set_language_code("en-IN");
        streaming_config.mutable_config()->add_alternative_language_codes("hi");
        

        // *streaming_config.mutable_config() = args.config;

        stream = client.AsyncStreamingRecognize();

        if (!stream->Start().get()){
            std::cout<<"Unable to open the stream"<<std::endl;
            stream->Finish();
        } 

        if (!stream->Write(request, grpc::WriteOptions{}).get()) {
            std::cout<<"Stream is closed"<<std::endl;
            stream->Finish();
        }

        std::cout<<"Connected"<<std::endl;

    }

    void finishStream(){
        auto status = stream->Finish().get();
         if (!status.ok()){
            std::cout<<"Error while closing stream"<<std::endl;
         }

        // write the output
        bool flag = false;
        while(!flag){
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            std::cout<<"Sleeping for 2 seconds"<<std::endl;
            for (auto response = stream->Read().get(); response.has_value(); response = stream->Read().get()) {

                std::cout<<"Here"<<std::endl;
                flag = true;

                for (auto const& result : response->results()) {
                    std::cout << "Result stability: " << result.stability() << "\n";
                    for (auto const& alternative : result.alternatives()) {
                        std::cout << alternative.confidence() << "\t"
                                << alternative.transcript() << "\n";
                    }
                }
            }
        }
        std::cout<<"Finished"<<std::endl;

    }

    void transcribe(SAMPLE *data, size_t len){
        google::cloud::speech::v1::StreamingRecognizeRequest request;
        request.set_audio_content(data, len);
        stream->Write(request, grpc::WriteOptions()).get();
    }
};


class Connection{
    public:
        Connection(){
            initConnection();
        }
        std::map<int, DataFormat> data_buffer;

        DataFormat deserialize(char *buffer){
            DataFormat data_recv;
            
            memcpy(&data_recv, buffer, sizeof(DataFormat));

            data_recv.data = new SAMPLE[data_recv.length];
            memcpy(data_recv.data, buffer + sizeof(DataFormat), sizeof(SAMPLE) * data_recv.length);

            data_recv.message = new char[data_recv.message_length];
            memcpy(data_recv.message, buffer + sizeof(DataFormat) + sizeof(SAMPLE) * data_recv.length, data_recv.message_length * sizeof(char));

            return data_recv;
        }

        void deleteData(DataFormat data){
            delete[] data.data;
            delete[] data.message;
        }

        int start(){
            std::cout<<"Listening..."<<std::endl;
            // Listen for incoming connections
            if (listen(serverSocket, 5) < 0) {
                std::cerr << "Listening failed\n";
                return 1;
            }

            sockaddr_in clientAddr;
            socklen_t clientAddrSize = sizeof(clientAddr);
            int clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrSize);
            if (clientSocket < 0) {
                std::cerr << "Accepting connection failed\n";
                return 1;
            }
            std::cout<<"ACCEPTED"<<std::endl;
            char buffer[BUFFER_SIZE];
            int bytesRead;

            std::string last_message;
            while ((bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0)) > 0) {
                // Process received data here, you can write it to a file, manipulate, etc.
                // For simplicity, let's just print it
                DataFormat data = deserialize(buffer);
                if(data.option != DATA_STREAM_CONTINUE){
                    std::cout<<"-----------------------------------------"<<std::endl;
                    std::cout<<"OPTION: "<<data.option<<std::endl;
                    std::cout<<"Data: ";
                    for(int i=0; i <10 && i < data.length; i++)
                        std::cout<<data.data[i];
                    std::cout<<std::endl;
                    std::cout<<"Message: "<<data.message<<std::endl;
                    std::cout<<std::endl;
                    std::cout<<std::endl;
                    std::cout<<"-----------------------------------------"<<std::endl;
                }
                data_buffer_mutex.lock();
                data_buffer[data.id] = data;
                data_buffer_mutex.unlock();

                send(clientSocket, "ACK0", 4, 0);
            }

            if (bytesRead < 0) {
                std::cerr << "Error reading from socket\n";
            }
            close(clientSocket);

            return 0;
        }

        void analyze_buffer(){
           SpeechRecognitionStream stream;
            while(run){
                if(data_buffer.contains(0)){
                    stream.startStream();
                    int i = 0;
                    std::cout<<"Starting analysis"<<std::endl;
                    while(true){
                        data_buffer_mutex.lock();
                        if(!data_buffer.contains(i)){
                            data_buffer_mutex.unlock();
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                            continue;
                        }
                        // int final = vosk_recognizer_accept_waveform_s(recognizer, data_buffer[i].data, data_buffer[i].length);
                        stream.transcribe(data_buffer[i].data, data_buffer[i].length);

                        if(data_buffer[i].option == DATA_STREAM_STOP){
                            // const char *s = vosk_recognizer_result(recognizer);
                            stream.finishStream();
                            std::cout<<"Analysis complete"<<std::endl;
                            data_buffer.erase(i);
                            data_buffer_mutex.unlock();
                            
                            break;
                        }
                        data_buffer.erase(i);
                        data_buffer_mutex.unlock();
                        i++;
                    }

                }
            }
        }

        int stop(){
            // Close sockets
            std::cout<<"Shutting down..."<<std::endl;
            close(serverSocket);
            return 0;
        }
        bool run;
    private:
        int serverSocket;
        int initConnection(){
            

            std::cout<<"Starting server socket on port 8080..."<<std::endl;
            serverSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (serverSocket < 0) {
                std::cerr << "Error creating socket\n";
                return 1;
            }
            
            int opt = 1;
            if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
                std::cerr << "Error setting socket option\n";
                return 1;
            }

            sockaddr_in serverAddr;
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(8080); 
            serverAddr.sin_addr.s_addr = INADDR_ANY;
            if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
                std::cerr << "Binding failed\n";
                return 1;
            }
            return 0;

        }


};

Connection* connection;

void terminationHandler(int signum){
    if(connection){
        connection->stop();
        delete connection;
    }

    exit(signum);
}

int main() {
    // VoskModel *model = vosk_model_new("../models/vosk-model-en-us");
    // VoskRecognizer *recognizer= vosk_recognizer_new(model, SAMPLE_RATE);

    signal(SIGINT, terminationHandler);
    connection = new Connection();
    connection->run = true;

    std::thread t1(&Connection::start, connection);
    std::thread t2(&Connection::start, connection);
    std::thread t3(&Connection::analyze_buffer, connection);
    
    t1.join();
    t2.join();
    
    connection->run = false;
    t3.join();

    connection->stop();
    delete connection;

    // vosk_recognizer_free(recognizer);
    // vosk_model_free(model);
    return 0;
}