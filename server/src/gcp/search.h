#pragma once
#include "../third-party/json.hpp"
#include "../logging/logger.h"

#include<string>
#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <curlpp/Exception.hpp>
#include <fstream>
#include <vector>
#include <sstream>
typedef struct {
    std::string user_type;
    std::string message;
} convo;

using json = nlohmann::json;

class GeminiSearch{
    const size_t max_history_length = 20;
    std::string persona_text;
    std::string url;
    std::vector<convo> history;
    json post_data;

    public:

    GeminiSearch(const char *config_file){
        std::ifstream persona_file(config_file);
        json data;
        try{
            data = json::parse(persona_file);
            LOG.log("Got config file for search");
        }catch(json::exception &e){
            std::cout<<"Unable to file"<< config_file;
        }
        
        std::string api_key = data["API_KEY"];

        url ="https://generativelanguage.googleapis.com/v1beta/models/gemini-1.0-pro:generateContent?key="+api_key;

        LOG.log("Search Url: ", url);

        this->persona_text = data["PERSONA_TEXT"];

        this->post_data["generationConfig"] = {
            {"temperature", 0.9},
            {"topK", 1},
            {"topP", 1},
            {"maxOutputTokens", 2048},
            {"stopSequences", json::array()}
        };

        this->post_data["safetySettings"] = {
            {
                {"category", "HARM_CATEGORY_HARASSMENT"},
                {"threshold", "BLOCK_MEDIUM_AND_ABOVE"}
            },
            {
                {"category", "HARM_CATEGORY_HATE_SPEECH"},
                {"threshold", "BLOCK_MEDIUM_AND_ABOVE"}
            },
            {
                {"category", "HARM_CATEGORY_SEXUALLY_EXPLICIT"},
                {"threshold", "BLOCK_MEDIUM_AND_ABOVE"}
            },
            {
                {"category", "HARM_CATEGORY_DANGEROUS_CONTENT"},
                {"threshold", "BLOCK_MEDIUM_AND_ABOVE"}
            }
        };

        LOG.log(this->post_data);

    }


    std::string ask(std::string query){
        LOG.log("startin the query: ", query);
        std::string final_prompt =  persona_text + query;

        LOG.log(this->post_data);
        this->post_data["contents"] = json::array();

        for(auto &elem : history){
            this->post_data["contents"].push_back({{"role", elem.user_type}, {"parts", {{"text", elem.message}}}});
        }

        LOG.log("Read the history");

        convo new_convo{"user", final_prompt};
        this->history.push_back(new_convo);

        this->post_data["contents"].push_back({{"role", new_convo.user_type}, {"parts", {{"text", new_convo.message}}}});
        LOG.log("Appended the query to history");

        std::string post_data_text = this->post_data.dump();

        LOG.log("Posting data: ", post_data_text);

        try {
            curlpp::Cleanup cleaner;
            curlpp::Easy request;
            
            request.setOpt(new curlpp::options::Url(this->url)); 
            // request.setOpt(new curlpp::options::Verbose(true)); 
            
            std::list<std::string> header; 
            header.push_back("Content-Type: application/json"); 
            
            request.setOpt(new curlpp::options::HttpHeader(header));

            request.setOpt(new curlpp::options::PostFields(post_data_text));
            request.setOpt(new curlpp::options::PostFieldSize(post_data_text.length()));
            
            std::stringstream response_stream;
            curlpp::options::WriteStream ws(&response_stream);
            request.setOpt(ws);

            request.perform(); 
            std::string model_message = json::parse(response_stream.str())["candidates"][0]["content"]["parts"][0]["text"];
            convo model_convo{"model", model_message};
            history.push_back(model_convo);
            if(history.size() > max_history_length){
                history.erase(history.begin(), history.begin()+2);
            }
            return model_message;
        }
        catch ( curlpp::LogicError & e ) {
            std::cout <<"Logic Error:" << e.what() << std::endl;
        }
        catch ( curlpp::RuntimeError & e ) {
            std::cout << "Runtime Error: " << e.what() << std::endl;
        }
        return std::string("");
    }

};