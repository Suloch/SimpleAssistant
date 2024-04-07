#pragma once

#include<vector>
#include<iostream>
#include<regex>
#include<optional>
#include "../gcp/search.h"
#include<random>

class Command{
    public:        
        virtual std::optional<std::string>execute(std::string text)=0;
};



class SwitchCommand: public Command{

    const std::string name = "switch";
    std::regex command_regex;
    std::optional<std::string> result;

    public:
        SwitchCommand(){
            command_regex = std::regex("(turn (on|off))|(switch (on|off))");
        }
        std::optional<std::string> execute(std::string text) override{
            if (std::regex_search(text, command_regex)) {
                
                result = "switching";
                LOG.log("Switching");
                return result;
            }

            
            result.reset();
            return result;
        }
} ;

class SearchCommand: public Command{
    const std::string name = "search";
    std::optional<std::string> result;
    GeminiSearch *g;
    public:
        SearchCommand(GeminiSearch *g){
            this->g = g;
        }
        std::optional<std::string> execute(std::string text) override{
            //sarcasm
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(1, 100);
            int random_number = dis(gen);

            if(random_number < 10){
                text = text + ". Reply sarcastically";
            }

            result = this->g->ask(text);
        
            LOG.log("Search Commnad||", text, " ||", result.value());

            return result;
        }
};


class CommandContext{
    std::vector<Command *>commands;
    GeminiSearch *g;
    public:
        
        CommandContext(const char * config_file){
            this->g = new GeminiSearch(config_file);
            commands.push_back(new SwitchCommand());
            commands.push_back(new SearchCommand(this->g));

        }

        std::string run(std::string text){
            for(auto &command: commands){
                auto result = command->execute(text);
                if(result.has_value()){
                    return result.value();
                }
            }
            return "";
        }

        ~CommandContext(){
            for(auto &command: commands){
                delete command;
            }

            delete g;
        }
};