#include "testCode.h"

#if NEED_PORT
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
    _say("Test Thread start !");
}

#define CONNECTION_TIMEOUT 60000

#define POST_PARAMS(url) \
    Post( \
        Url{url}, \
        ConnectTimeout{CONNECTION_TIMEOUT}, \
        Timeout{config<int>(TIMEOUT)} \
    )
#define POST_PARAMS_ID(url) \
    Post( \
        Url{url}, \
        Parameters{{URL_PARAMS_CLIENT_ID,id}}, \
        ConnectTimeout{CONNECTION_TIMEOUT}, \
        Timeout{config<int>(TIMEOUT)} \
    )
#define OUTPUT(output,category) \
    if (fileExists(output)) { \
        deleteConfig(output,true); \
    } \
    if (storeJson(category,output,json)) { \
        saveToFile(category,output); \
        storeJson(category,output,nullptr,true); \
    }else warn(CANNOT_SAVE output);
#define CANNOT_SAVE "Can not save to file !!! Now saving: "
#define GET_ALL_CATEGORIES_OUTPUT OUTPUT_DIRECTORY GET_ALL_CATEGORIES ".json"
#define KEY_OUTPUT OUTPUT_DIRECTORY KEY ".json"
#define EXCHANGE KEY "_exchange"
#define EXCHANGE_KEY_OUTPUT OUTPUT_DIRECTORY EXCHANGE ".json"
#define TEST_ID_OUTPUT OUTPUT_DIRECTORY TEST_ID ".json"
#define TEST_WRONG_ID_OUTPUT OUTPUT_DIRECTORY TEST_ID "_wrong" ".json"
#define LOGIN_OUTPUT OUTPUT_DIRECTORY LOGIN ".json"
#define MATH "math"
#define MATH_OUTPUT OUTPUT_DIRECTORY "/" MATH ".json"
#define DEBUG "test"

void test() {
    string localhost = "http://localhost:";
    bool error = false;
    string id;
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
        OUTPUT(GET_ALL_CATEGORIES_OUTPUT,GET_ALL_CATEGORIES_NO_SLASH);

        response = POST_PARAMS(localhost + KEY);
        if (response.status_code != 200) {
            error = true;
            warn("Get key failed ! Error: ",false);
            warn(response.error.message.c_str());
        }
        _say("Get key: ");
        _say(response.text);
        json = Json::parse(response.text);
        if (json.empty()) {
            error = true;
            EMPTY_WARN("Get key");
        }
        OUTPUT(KEY_OUTPUT,KEY_NO_SLASH);
        string key = json[URL_PARAMS_ENCRYPT_KEY];
        key = webAPI::SimpleRSA::encrypt(key,"7F3K9M2Q8Z1T5H6J4N0P8R2X6W9B3C7D");
        auto esa = webAPI::SimpleESA(key);// It will automatically decrypt

        json.clear();
        json[URL_PARAMS_ENCRYPT_KEY] = key;
        response = Post(
            Url{localhost + KEY},
            Body{json.dump()},
            ConnectTimeout{CONNECTION_TIMEOUT},
            Timeout{config<int>(TIMEOUT)}
        );
        if (response.status_code != 200) {
            error = true;
            warn("Exchange key failed ! Error: ",false);
            warn(response.error.message.c_str());
        }
        _say("Get id: ");
        _say(esa.decrypt(response.text));
        json = Json::parse(esa.decrypt(response.text));
        if (json.empty()) {
            error = true;
            EMPTY_WARN("Exchange key");
        }
        OUTPUT(EXCHANGE_KEY_OUTPUT,EXCHANGE);
        id = json[URL_PARAMS_CLIENT_ID];

        _say("Test Right ID");
        response = POST_PARAMS_ID(localhost + TEST_ID);
        if (response.status_code != 200) {
            error = true;
            warn("Test id failed ! Error: ",false);
            warn(response.error.message.c_str());
        }
        _say("Test right id: ");
        if (response.text.empty()) {
            error = true;
            EMPTY_WARN("Test right id");
        }else _say(response.text);
        json.clear();
        json[DEBUG] = response.text;
        OUTPUT(TEST_ID_OUTPUT,TEST_ID_NO_SLASH);

        _say("Test Wrong ID");
        response = POST_PARAMS(localhost + TEST_ID);
        if (response.status_code == 200) {
            error = true;
            warn("Test id failed ! Get 200 !",false);
        }
        json[DEBUG] = response.text;
        OUTPUT(TEST_WRONG_ID_OUTPUT,TEST_ID_NO_SLASH "_wrong");

        response = POST_PARAMS_ID(localhost + LOGIN);
        if (response.status_code != 200) {
            error = true;
            warn("Login failed ! Error: ",false);
            warn(response.error.message.c_str());
        }
        _say("Login: ");
        _say(esa.decrypt(response.text));
        if (response.text.empty()) {
            error = true;
            EMPTY_WARN("Login");
        }
        json[DEBUG] = response.text;
        OUTPUT(LOGIN_OUTPUT,LOGIN_NO_SLASH);

        response = POST_PARAMS_ID(localhost + INIT);
        if (response.status_code != 200) {
            error = true;
            warn("Init client failed ! Error: ",false);
            warn(response.error.message.c_str());
        }

        response = Post(
            Url{localhost},
            Parameters{
                {URL_PARAMS_CLIENT_ID,id},
                {URL_PARAMS_CATEGORY,MATH}
            },
            ConnectTimeout{CONNECTION_TIMEOUT},
            Timeout{config<int>(TIMEOUT)}
        );
        if (response.status_code != 200) {
            error = true;
            warn("Crawl for math failed ! Error: ",false);
            warn(response.error.message.c_str());
        }
        _say("Crawl for math get: ");
        _say(response.text);
        json = Json::parse(esa.decrypt(response.text));
        if (json.empty()) {
            error = true;
            EMPTY_WARN("Crawl for math");
        }
        OUTPUT(MATH_OUTPUT,MATH);
    }catch (std::exception &e) {
        testFinished = true;
        POST_PARAMS_ID(localhost);
        throw e;
    }
    if (error)
        throwError("Test encountered an error !");
    else _say("Test Success !!! Now returning ...");
    testFinished = true;
    POST_PARAMS_ID(localhost);// To let main thread get out from listening port
}

#endif