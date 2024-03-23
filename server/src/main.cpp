#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <csignal>
#include <cstdlib>
#include<sstream>

typedef short SAMPLE;
#define SAMPLE_RATE  (44100)
#define NUM_SECONDS (5)

const int BUFFER_SIZE = SAMPLE_RATE * NUM_SECONDS * sizeof(short);

class Connection{
    public:
        Connection(){
            initConnection();
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
            clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrSize);
            if (clientSocket < 0) {
                std::cerr << "Accepting connection failed\n";
                return 1;
            }

            char buffer[BUFFER_SIZE];
            int bytesRead;

            std::string last_message;
            while ((bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0)) > 0) {
                // Process received data here, you can write it to a file, manipulate, etc.
                // For simplicity, let's just print it
                std::string message = std::string(buffer, bytesRead);
                std::cout << "Received: " << message  << std::endl;

            }

            // Check for errors or end of connection
            if (bytesRead < 0) {
                std::cerr << "Error reading from socket\n";
            }

            return 0;
        }

        int stop(){
            // Close sockets
            std::cout<<"Shutting down..."<<std::endl;
            close(clientSocket);
            close(serverSocket);
            return 0;
        }

    private:
        int clientSocket;
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

             // Bind the socket to an address
            sockaddr_in serverAddr;
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(8080); // Choose your desired port
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

    signal(SIGINT, terminationHandler);
    connection = new Connection();
    while (true)
    {
        connection->start();
    }
    

    connection->stop();
    delete connection;
    return 0;
}