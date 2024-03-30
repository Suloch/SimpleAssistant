#include <iostream>
#include <cstring>
#include <unistd.h>
#include <csignal>
#include <cstdlib>
#include<sstream>
#include<thread>
#include<map>
#include<chrono>
#include<mutex>
#include "connection.h"

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

    return 0;
}