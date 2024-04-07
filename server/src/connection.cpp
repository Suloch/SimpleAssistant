#include<map>
#include<iostream>
#include <cstring>
#include "connection.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>

#include "connection.h"
#include "commands/command.h"
#include "gcp/google_speech_2_text.h"

Connection::Connection(){
    initConnection();
}

DataFormat Connection::deserialize(char *buffer){
    DataFormat data_recv;
    
    memcpy(&data_recv, buffer, sizeof(DataFormat));

    data_recv.data = new SAMPLE[data_recv.length];
    memcpy(data_recv.data, buffer + sizeof(DataFormat), sizeof(SAMPLE) * data_recv.length);

    data_recv.message = new char[data_recv.message_length];
    memcpy(data_recv.message, buffer + sizeof(DataFormat) + sizeof(SAMPLE) * data_recv.length, data_recv.message_length * sizeof(char));

    return data_recv;
}

void Connection::deleteData(DataFormat data){
    delete[] data.data;
    delete[] data.message;
}

int Connection::start(){
    while(run){
        std::cout<<"Listening..."<<std::endl;
        if (listen(serverSocket, 5) < 0) {
            std::cerr << "Listening failed\n";
            return 1;
        }

        sockaddr_in clientAddr;
        socklen_t clientAddrSize = sizeof(clientAddr);
        clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrSize);
        if (clientSocket < 0) {
            std::cerr << "Accepting connection failed\n";
            return 1;
        }
        std::cout<<"ACCEPTED"<<std::endl;
        char buffer[this->BUFFER_SIZE];
        int bytesRead;

        std::string last_message;
        while ((bytesRead = recv(clientSocket, buffer, this->BUFFER_SIZE, 0)) > 0) {

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
            this->data_buffer_mutex.lock();
            data_buffer[data.id] = data;
            this->data_buffer_mutex.unlock();

            send(clientSocket, "ACK0", 4, 0);
        }

        if (bytesRead < 0) {
            std::cerr << "Error reading from socket\n";
        }
        close(clientSocket);
    }

    return 0;
}


void Connection::analyze_buffer(){
    // SpeechToTextStream stream;
    // CommandContext c("auth/night.json");

    // while(run){
    //     if(data_buffer.contains(0)){
    //         stream.start();
    //         int i = 0;
    //         std::cout<<"Starting analysis"<<std::endl;
    //         while(true){
    //             data_buffer_mutex.lock();
    //             if(!data_buffer.contains(i)){
    //                 data_buffer_mutex.unlock();
    //                 std::this_thread::sleep_for(std::chrono::milliseconds(100));
    //                 continue;
    //             }
    //             stream.write(data_buffer[i].data, data_buffer[i].length*sizeof(SAMPLE));

    //             if(data_buffer[i].option == DATA_STREAM_STOP){
    //                 this->send_response(c.run(stream.finish())) ;
                    
    //                 std::cout<<"Analysis complete"<<std::endl;
    //                 data_buffer.erase(i);
    //                 data_buffer_mutex.unlock();
                    
    //                 break;
    //             }
    //             data_buffer.erase(i);
    //             data_buffer_mutex.unlock();
    //             i++;
    //         }

    //     }
    // }
}

int Connection::stop(){
    // Close sockets
    std::cout<<"Shutting down..."<<std::endl;
    close(serverSocket);
    return 0;
}

int Connection::initConnection(){
    

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

std::vector<std::vector<char>> divideIntoChunks(const std::vector<char>& byteVector, size_t chunkSize) {
    std::vector<std::vector<char>> chunks;
    size_t numChunks = byteVector.size() / chunkSize;
    size_t remainder = byteVector.size() % chunkSize;

    // Iterate through the vector and split it into chunks
    for (size_t i = 0; i < numChunks; ++i) {
        chunks.push_back(std::vector<char>(byteVector.begin() + i * chunkSize, byteVector.begin() + (i + 1) * chunkSize));
    }

    // If there's a remainder, add one more chunk
    if (remainder > 0) {
        chunks.push_back(std::vector<char>(byteVector.end() - remainder, byteVector.end()));
    }

    return chunks;
}

std::string serialize_response_data(){
    returbn "";
}

void Connection::send_response(std::string data){
    // std::vector<char> bytes(data.at(44), data.end()); //skip the wav header


    // send data in chunks of 64KB
    // size_t chunkSize = 64 * 1024;
    // std::vector<std::vector<char>> chunks = divideIntoChunks(bytes, chunkSize);

    // for(auto &chunk: chunks){
    //     ResponseData r_data{PLAYER_STREAM, chunk.size(), chunk.data()};

    //     if (send(clientSocket, message, strlen(message), 0) < 0) {
    //                 std::cerr << "Error sending data\n";
    //     }
    // }
}

