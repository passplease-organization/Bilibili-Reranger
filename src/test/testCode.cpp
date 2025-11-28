#include "testCode.h"
#include "Util.h"
#include "config.h"
#include <cpr/cpr.h>
#include <thread>
#include "../subFeatures/requestHelper.h"
#include "BilibiliInterface.h"

using namespace cpr;

atomic<bool> testFinished = false;

inline void _say(string msg,bool endl = true) {
    say(msg.c_str(),endl,GREEN);
}

#define EMPTY_WARN(task) warn("Empty Response for " task " !");

void startTestThread() {
    thread testThread(test);
    testThread.detach();
    _say("Test Thread started !");
}

#define POST_PARAMS(url) \
    Post( \
        Url{url}, \
        ConnectTimeout{60000}, \
        Timeout{config<int>(TIMEOUT)} \
    )
#define CANNOT_SAVE "Can not save to file !!! Now saving: "
#define GET_ALL_CATEGORIES_OUTPUT OUTPUT_DIRECTORY GET_ALL_CATEGORIES ".json"
#define MATH "math"
#define MATH_URL "/?" URL_PARAMS_CATEGORY "=" MATH
#define MATH_OUTPUT OUTPUT_DIRECTORY "/" MATH ".json"

void test() {
    string localhost = "http://localhost:";
    bool error = false;
    try {
        sleep(5);// Waiting for working thread
        localhost += to_string(config<int>(PORT));

        Response response = POST_PARAMS(localhost + GET_ALL_CATEGORIES);
        if (response.status_code != 200) {
            error = true;
            warn("Get all categories failed ! Error: ",false);
            warn(response.error.message.c_str());
        }
        _say("All categories get: ");
        _say(response.text);
        Json json = Json::parse(response.text);
        if (json.empty()) {
            error = true;
            EMPTY_WARN("Get all categories");
        }
        if(fileExists(GET_ALL_CATEGORIES_OUTPUT)){
            deleteConfig(GET_ALL_CATEGORIES_OUTPUT,true);
        }
        if (storeJson(GET_ALL_CATEGORIES_NO_SLASH,GET_ALL_CATEGORIES_OUTPUT,json)) {
            saveToFile(GET_ALL_CATEGORIES_NO_SLASH,GET_ALL_CATEGORIES_OUTPUT);
            storeJson(GET_ALL_CATEGORIES_NO_SLASH,GET_ALL_CATEGORIES_OUTPUT,nullptr,true);
        }else warn(CANNOT_SAVE GET_ALL_CATEGORIES_OUTPUT);

        response = POST_PARAMS(localhost + MATH_URL);
        if (response.status_code != 200) {
            error = true;
            warn("Crawl for math failed ! Error: ",false);
            warn(response.error.message.c_str());
        }
        _say("Crawl for math get: ");
        _say(response.text);
        json = Json::parse(response.text);
        if (json.empty()) {
            error = true;
            EMPTY_WARN("Crawl for math");
        }
        if (fileExists(MATH_OUTPUT)) {
            deleteConfig(MATH_OUTPUT,true);
        }
        if (storeJson(MATH,MATH_OUTPUT,json)) {
            saveToFile(MATH,MATH_OUTPUT);
            storeJson(MATH,MATH_OUTPUT,nullptr,true);
        }else warn(CANNOT_SAVE MATH_OUTPUT);

    }catch (std::exception &e) {
        testFinished = true;
        Post(
            Url{localhost},
            ConnectTimeout{1000},
            Timeout{config<int>(TIMEOUT)}
        );
        throw e;
    }
    if (error)
        throwError("Test encountered an error !");
    else _say("Test Success !!! Now returning ...");
    testFinished = true;
    POST_PARAMS(localhost);// To let main thread get out from listening port
}