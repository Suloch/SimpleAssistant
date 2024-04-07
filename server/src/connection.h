
#pragma once

#include<map>
#include<mutex>

typedef short SAMPLE;
typedef unsigned long int size_t;

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

#define SAMPLE_RATE  (44100)
#define NUM_SECONDS (5)

typedef enum{
    ACK_0, // acknowledgements
    ACK_1, 
    PLAYER_STREAM
}ResponseType;

typedef struct {
    ResponseType type;
    size_t length;
    void *data;
} ResponseData;



class Connection{
    public:
        std::mutex data_buffer_mutex;
        std::map<int, DataFormat> data_buffer;
        DataFormat deserialize(char *buffer);

        Connection();
        void deleteData(DataFormat data);
        int start();
        void analyze_buffer();
        bool run;
        int stop();
        void send_response();

    private:
        const int BUFFER_SIZE = sizeof(DataFormat) + sizeof(char) *100 + SAMPLE_RATE * NUM_SECONDS * sizeof(SAMPLE);
        int serverSocket;
        int initConnection();


};