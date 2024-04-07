#include "../src/commands/command.h"

#include<iostream>
#include<optional>

int main(){
    LOG.start(debug, "log.txt");
    LOG.log("here");

    CommandContext c("../auth/night.json");

    std::string text("turn on the lights");

    std::cout<<"Text: "<<text<<std::endl;
    std::cout<<"Result: "<< c.run(text)<<std::endl;

    text = "How to reverse a linked list";
    std::cout<<"Text: "<<text<<std::endl;
    std::cout<<"Result: "<< c.run(text)<<std::endl;
    
    LOG.stop();
    return 0;
}