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
#include "logging/logger.h"

Connection* connection;

void terminationHandler(int signum){
    if(connection){
        connection->stop();
        delete connection;
    }

    exit(signum);
}

int main() {
    LOG.start(debug, "log.txt");

    signal(SIGINT, terminationHandler);
    connection = new Connection();
    connection->run = true;

    std::thread t1(&Connection::start, connection);
    // std::thread t2(&Connection::start, connection);
    std::thread t3(&Connection::analyze_buffer, connection);
    
    getchar();
    
    connection->run = false;
    
    t1.join();
    // t2.join();
    
    t3.join();

    connection->stop();
    delete connection;
    LOG.stop();
    return 0;
}