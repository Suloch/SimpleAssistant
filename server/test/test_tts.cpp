

#include "../src/gcp/google_speech_2_text.h"
#include<iostream>
#include "../src/logging/logger.h"

int main(){
    LOG.start(debug, "log.txt");
    LOG.log("here");
    std::string test_1 = "<speak>Hello, what's up? </speak>";
    auto audio = tts(test_1.c_str(), test_1.length());
    LOG.stop();
    return 0;
}