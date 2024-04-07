#include "../src/gcp/search.h"

#include <fstream>
#include <iostream>
#include <chrono>
#include <thread>

using json = nlohmann::json;


int main(){
    LOG.start(debug, "log.txt");

    GeminiSearch g("../auth/night.json");

    

    std::cout<<g.ask("Hi, what's your name?")<<std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));

    std::cout<<g.ask("how to dance")<<std::endl;
    LOG.stop();

    
    return 0;

}