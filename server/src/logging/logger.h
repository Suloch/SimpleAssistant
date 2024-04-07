#pragma once

#include <fstream>
#include <iostream>
#define LOG Logger::getInstance()

enum LogLevel{
    info,
    debug,
    warn,
    error
};

class Logger{
    
    public:
        static Logger& getInstance(){
            static Logger logger;
            return logger;
        }
    private:
        int line = 0;
        bool started;
        LogLevel level;
        bool cerr;
        std::ofstream file;

        Logger(){}

    public:
        template<typename T>
        void _log(T msg){
            if(cerr){
            std::cerr<<msg<<std::endl;
            }else{
                file<<msg<<std::endl;
            }
        }

        template<typename T, typename... Args>
        void _log(T msg, Args... args){
            if(cerr){
            std::cerr<<msg;
            }else{
                file<<msg;
            }
            _log(args...);
        }

        template<typename T, typename... Args>
        void log(T msg, Args... args){
            if(cerr){
            std::cerr<<line<<": "<<level<<": ";
            }else{
                file<<line<<": "<<level<<": ";
            }
            _log(msg, args...);
            line++;
        }

        void start(LogLevel level, const char * filename){
            if(filename == nullptr){
                cerr = true;
            }else{
                file.open(filename);
                file.clear();
                cerr = false;
            }
            this->level = level;
        }

        void stop(){
            if(!cerr){
                file.close();
            }
        }
};
