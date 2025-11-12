#include "CallAI.h"
#include <nlohmann/json.hpp>
#include "cpr/cpr.h"
#include "../config.h"

using namespace std;
using Json = nlohmann::json;

AI::AI(const char *url, const char *key) {
    this -> url = url;
    this -> key = key;
    temperature = 1;
    maxTokens = config<int>(MAX_AI_TOKENS);
}

const char* post(AI& ai,const string& body){
    if(ai.getURLS().empty()){
        return "";
    }
    cpr::Response response = cpr::Post(
            cpr::Url{ai.getURLS()},
            cpr::Header{
                    {"Content-Type", "application/json"},
                    {"Authorization", "Bearer " + ai.getKeyS()}
            },
            cpr::Body{body}
    );
    if(response.status_code != 200){
        warn("Request AI failed, status code: ",false);
        warn(to_string(response.status_code).c_str());
        warn("Error message: ");
        warn(response.text.c_str());
        return "";
    }
    try {
        Json response_json = Json::parse(response.text);

        if (response_json.contains("choices") && !response_json["choices"].empty()) {
            return to_string(response_json["choices"][0]["message"]["content"]).c_str();
        }
        warn("Invalid JSON response format.");
        warn("Full response: ",false);
        warn(response.text.c_str());
        return "";
    } catch (const Json::parse_error& e) {
        warn("Failed to parse JSON response: ");
        warn(e.what());
        return "";
    }
}

const char* request(AI& ai, const char* message, const char* system){
    return post(ai,ai.toString(message,system));
}

const char *AI::toString(const char* msg, const char* _system) {
    string message(msg);
    Json request;
    request["model"] = model;
    string system;
    system = _system == nullptr ? "You are helping me to understand what this video is about." : _system;
    request["message"] = Json::array({
                                             {{"role","system"},{"content",message}},
                                             {{"role","user"},{"content",system}}
                                     });
    request["temperature"] = temperature;
    request["max_tokens"] = maxTokens;
    return to_string(request).c_str();
}

const char* introduceVideo(AI& ai, bilibili::Video& video){
    return post(ai,ai.toString(video));
}

const char *AI::toString(bilibili::Video &video) {
    string message = "Could you tell me what this video about in summary and by Chinese? The video title is ";
    message += video.title();
    message += "and it's description is ";
    message += video.description();
    return toString(message.c_str(),"You are well at understanding video and summerize it.");
}

const char *AI::getURL() const {
    return url.c_str();
}

const char *AI::getKey() const {
    return key.c_str();
}

const std::string& AI::getURLS() const{
    return url;
}

const std::string& AI::getKeyS() const {
    return key;
}

void AI::setURLS(const std::string& url) {
    this -> url = url;
}

void AI::setURL(const char *url) {
    this -> url = url;
}

void AI::setKeyS(const std::string& key) {
    this -> key = key;
}

void AI::setKey(const char *key) {
    this -> key = key;
}

const char *AI::getModel() const {
    return model.c_str();
}

void AI::setModel(const char *model) {
    this -> model = model;
}