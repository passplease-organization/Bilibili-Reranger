#include "testCode.h"
#include "Util.h"
#include "config.h"
#include <cpr/cpr.h>
#include <thread>
#include "../subFeatures/requestHelper.h"
#include "BilibiliInterface.h"

using namespace cpr;

atomic<bool> testFinished = false;

void startTestThread() {
    thread testThread(test);
    testThread.detach();
    say("Test Thread started !",true,GREEN);
}

#define CANNOT_SAVE "Can not save to file !!! Now saving: "
#define GET_ALL_CATEGORIES_OUTPUT OUTPUT_DIRECTORY GET_ALL_CATEGORIES ".json"

inline void _say(string msg,bool endl = true) {
    say(msg.c_str(),endl,GREEN);
}

void test() {
    string localhost = "http://localhost:";
    try {
        sleep(5);// Waiting for working thread
        localhost += to_string(config<int>(PORT));
        Response response = Post(
            Url{localhost + GET_ALL_CATEGORIES},
            ConnectTimeout{1000},
            Timeout{config<int>(TIMEOUT)}
        );
        bool error = false;
        if (response.status_code != 200) {
            error = true;
            warn("Get all categories failed ! Error: ",false);
            warn(response.error.message.c_str());
        }
        _say("All categories get: ");
        _say(response.text);
        const Json json = Json::parse(response.text);
        if(fileExists(GET_ALL_CATEGORIES_OUTPUT)){
            deleteConfig(GET_ALL_CATEGORIES_OUTPUT,true);
        }
        if (storeJson(GET_ALL_CATEGORIES_NO_SLASH,GET_ALL_CATEGORIES_OUTPUT,json)) {
            saveToFile(GET_ALL_CATEGORIES_NO_SLASH,GET_ALL_CATEGORIES_OUTPUT);
            storeJson(GET_ALL_CATEGORIES_NO_SLASH,GET_ALL_CATEGORIES_OUTPUT,nullptr,true);
        }else warn(CANNOT_SAVE GET_ALL_CATEGORIES_OUTPUT);
        testFinished = true;
        Post(// To let main thread get out from listening port
            Url{localhost},
            ConnectTimeout{1000},
            Timeout{config<int>(TIMEOUT)}
        );
    }catch (std::exception &e) {
        testFinished = true;
        Post(
            Url{localhost},
            ConnectTimeout{1000},
            Timeout{config<int>(TIMEOUT)}
        );
        throw e;
    }
}