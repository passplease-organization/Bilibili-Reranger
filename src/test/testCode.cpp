#include "testCode.h"

#include "Util.h"
#include "config.h"
#include <cpr/cpr.h>
#include <thread>
#include "../subFeatures/requestHelper.h"
#include "BilibiliInterface.h"
#include "webAPIs/socialAPI.h"
#include "webAPIs/platforms.h"

using namespace cpr;

atomic<bool> testFinished = false;

inline void _say(string msg,bool endl = true) {
    cppUtil::say({endl, GREEN}, msg);
}

#define EMPTY_WARN(task) cppUtil::warn("Empty Response for " task " !");

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
    }else cppUtil::warn(CANNOT_SAVE output);
#define CANNOT_SAVE "Can not save to file !!! Now saving: "
#define GET_ALL_CATEGORIES_OUTPUT OUTPUT_DIRECTORY GET_ALL_CATEGORIES ".json"
#define KEY_OUTPUT OUTPUT_DIRECTORY KEY ".json"
#define EXCHANGE KEY "_exchange"
#define EXCHANGE_KEY_OUTPUT OUTPUT_DIRECTORY EXCHANGE ".json"
#define TEST_ID_OUTPUT OUTPUT_DIRECTORY TEST_ID ".json"
#define TEST_WRONG_ID_OUTPUT OUTPUT_DIRECTORY TEST_ID "_wrong" ".json"
#define ADMIN_LOGIN_OUTPUT OUTPUT_DIRECTORY "/" ADMIN_CLIENT_KEY ".json"
#define LOGIN_OUTPUT OUTPUT_DIRECTORY LOGIN ".json"
#define LOGIN_STATUS_OUTPUT OUTPUT_DIRECTORY "/" LOGIN_NO_SLASH "_status.json"
#define INIT_OUTPUT OUTPUT_DIRECTORY INIT ".json"
#define SET_OUTPUT OUTPUT_DIRECTORY SET ".json"
#define MATH "math"
#define MATH_OUTPUT OUTPUT_DIRECTORY "/" MATH ".json"
#define DEBUG "test"

namespace {
    void markFailed(bool& error, const string& message) {
        error = true;
        cppUtil::warn(message);
    }

    string decryptIfNeeded(const string& text, webAPI::SimpleESA* esa = nullptr) {
        if (esa == nullptr)
            return text;
        return esa -> decrypt(text);
    }

    Json parseJsonChecked(const string& text, bool& error, const string& step) {
        try {
            return Json::parse(text);
        } catch (const std::exception& e) {
            markFailed(error, step + " returned invalid json");
            cppUtil::warn(e.what());
            cppUtil::warn(text);
            return Json();
        }
    }

    void expectStatusCode(const Response& response, const int expected, bool& error, const string& step) {
        if (response.status_code == expected)
            return;
        error = true;
        cppUtil::warn({false, nullptr}, step);
        cppUtil::warn(" failed, status code: ");
        cppUtil::warn(response.status_code);
        if (!response.error.message.empty()) {
            cppUtil::warn({false, nullptr}, "Error message: ");
            cppUtil::warn(response.error.message);
        }
        if (!response.reason.empty()) {
            cppUtil::warn({false, nullptr}, "Reason: ");
            cppUtil::warn(response.reason);
        }
        if (!response.text.empty()) {
            cppUtil::warn({false, nullptr}, "Response text: ");
            cppUtil::warn(response.text);
        }
    }

    void expectTextEquals(const string& actual, const string& expected, bool& error, const string& step) {
        if (actual == expected)
            return;
        error = true;
        cppUtil::warn({false, nullptr}, step);
        cppUtil::warn(" unexpected text: ");
        cppUtil::warn(actual);
    }
}

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
            cppUtil::warn({false, nullptr}, "Get all categories failed ! Error: ");
            cppUtil::warn(response.error.message);
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
            cppUtil::warn({false, nullptr}, "Get key failed ! Error: ");
            cppUtil::warn(response.error.message);
        }
        _say("Get key: ");
        _say(response.text);
        json = Json::parse(response.text);
        if (json.empty()) {
            error = true;
            EMPTY_WARN("Get key");
        }
        OUTPUT(KEY_OUTPUT,KEY_NO_SLASH);
        string key = json[BODY_PARAMS_ENCRYPT_KEY];
        key = webAPI::SimpleRSA::encryptSodium(key,"7F3K9M2Q8Z1T5H6J4N0P8R2X6W9B3C7D");
        auto esa = webAPI::SimpleESA(key);// It will automatically decrypt

        json.clear();
        json[BODY_PARAMS_ENCRYPT_KEY] = key;
        response = Post(
            Url{localhost + KEY},
            Body{json.dump()},
            ConnectTimeout{CONNECTION_TIMEOUT},
            Timeout{config<int>(TIMEOUT)}
        );
        if (response.status_code != 200) {
            error = true;
            cppUtil::warn({false, nullptr}, "Exchange key failed ! Error: ");
            cppUtil::warn(response.error.message);
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
        expectStatusCode(response,200,error,"Test right id");
        _say("Test right id: ");
        {
            const string decrypted = decryptIfNeeded(response.text,&esa);
            if (decrypted.empty()) {
                error = true;
                EMPTY_WARN("Test right id");
            } else {
                _say(decrypted);
                expectTextEquals(decrypted,"ID still valid !",error,"Test right id");
            }
            json.clear();
            json[DEBUG] = decrypted;
        }
        OUTPUT(TEST_ID_OUTPUT,TEST_ID_NO_SLASH);

        _say("\nTest Wrong ID: ");
        response = POST_PARAMS(localhost + TEST_ID);
        if (response.status_code == 200)
            markFailed(error,"Test wrong id should not return 200");
        json.clear();
        json[DEBUG] = response.text;
        OUTPUT(TEST_WRONG_ID_OUTPUT,TEST_ID_NO_SLASH "_wrong");

        _say("Admin Login: ");
        json.clear();
        json[BODY_PARAMS_ENCRYPT_KEY] = esa.getKey("");
        _say("Now esa key: ",false);
        _say(esa.getKey(""));
        json[BODY_PARAMS_ADMIN] = "test";
        response = Post(
            Url{localhost + KEY},
            Parameters{{URL_PARAMS_CLIENT_ID,id}},
            Body{esa.encrypt(json.dump())},
            ConnectTimeout{CONNECTION_TIMEOUT},
            Timeout{config<int>(TIMEOUT)}
        );
        if (response.status_code != 200) {
            error = true;
            cppUtil::warn({false, nullptr}, "Admin login failed ! Error: ");
            cppUtil::warn(response.error.message);
        }
        _say("Admin login: ");
        {
            const string decrypted = esa.decrypt(response.text);
            _say(decrypted);
            if (decrypted.empty()) {
                error = true;
                EMPTY_WARN("Admin login");
            }
            json = Json::parse(decrypted);
        }
        OUTPUT(ADMIN_LOGIN_OUTPUT,ADMIN_CLIENT_KEY);

        _say("Set platform:");
        response = Post(
            Url{localhost + SET},
            Parameters{
                {URL_PARAMS_CLIENT_ID,id},
                {BODY_PARAMS_PLATFORM,BILIBILI}
            },
            ConnectTimeout{CONNECTION_TIMEOUT},
            Timeout{config<int>(TIMEOUT)}
        );
        expectStatusCode(response,200,error,"Set platform");
        {
            const string decrypted = decryptIfNeeded(response.text,&esa);
            _say("Set: ");
            _say(decrypted);
            expectTextEquals(decrypted,"设置成功",error,"Set platform");
            json.clear();
            json[DEBUG] = decrypted;
            OUTPUT(SET_OUTPUT,SET_NO_SLASH);
        }

#if ALL_CONTAINER_ONLINE
        _say("Login status:");
        response = Post(
            Url{localhost + LOGIN},
            Parameters{
                {URL_PARAMS_CLIENT_ID,id},
                {URL_PARAMS_TEST,"true"}
            },
            ConnectTimeout{CONNECTION_TIMEOUT},
            Timeout{config<int>(TIMEOUT)}
        );
        if (response.status_code != 200 && response.status_code != 500)
            expectStatusCode(response,200,error,"Login status");
        {
            const string decrypted = decryptIfNeeded(response.text,&esa);
            _say("Login status: ");
            _say(decrypted);
            if (decrypted != "有效COOKIE" && decrypted != "无效COOKIE")
                markFailed(error,"Login status returned unexpected text");
            json.clear();
            json[DEBUG] = decrypted;
            OUTPUT(LOGIN_STATUS_OUTPUT,LOGIN_NO_SLASH "_status");
        }
#endif

        _say("Login:");
        json.clear();
        json[BODY_PARAMS_PLATFORM] = BILIBILI;
        auto&& screen = json["screen"];
        screen["width"] = 1000;
        screen["height"] = 800;
        screen["depth"] = 16;
        response = Post(
            Url{localhost + LOGIN},
            Body{esa.encrypt(json.dump())},
            Parameters{{URL_PARAMS_CLIENT_ID,id}},
            ConnectTimeout{CONNECTION_TIMEOUT},
            Timeout{config<int>(TIMEOUT)}
        );
#if ALL_CONTAINER_ONLINE
        expectStatusCode(response,200,error,"Login");
        {
            const string decrypted = decryptIfNeeded(response.text,&esa);
            _say("Login: ");
            _say(decrypted);
            if (decrypted.empty()) {
                error = true;
                EMPTY_WARN("Login");
            }
            json = parseJsonChecked(decrypted,error,"Login");
            if (!json.contains("success") || !json["success"].is_boolean())
                markFailed(error,"Login missing boolean success");
            if (!json.contains("url") || !json["url"].is_string())
                markFailed(error,"Login missing string url");
            if (json.value("success", false) && json.value("url", string()).empty())
                markFailed(error,"Login success is true but url is empty");
            json[DEBUG] = decrypted;
        }
#else
        expectStatusCode(response,200,error,"Login");
        {
            const string decrypted = decryptIfNeeded(response.text,&esa);
            _say("Login: ");
            _say(decrypted);
            expectTextEquals(decrypted,"测试成功",error,"Offline login");
            json.clear();
            json[DEBUG] = decrypted;
        }
#endif
        OUTPUT(LOGIN_OUTPUT,LOGIN_NO_SLASH);

        _say("Init:");
        response = POST_PARAMS_ID(localhost + INIT);
        {
            const string decrypted = decryptIfNeeded(response.text,&esa);
            _say("Init: ");
            _say(decrypted);
#if ALL_CONTAINER_ONLINE
            if (response.status_code != 200 && response.status_code != 500)
                markFailed(error,"Init returned unexpected status code");
            if (decrypted != "准备过程完成" && decrypted != "准备过程失败")
                markFailed(error,"Init returned unexpected text");
#else
            expectStatusCode(response,200,error,"Init");
            expectTextEquals(decrypted,"准备过程完成",error,"Offline init");
#endif
            json.clear();
            json[DEBUG] = decrypted;
            OUTPUT(INIT_OUTPUT,INIT_NO_SLASH);
        }

        _say("Crawl( will fail ):");
        response = Post(
            Url{localhost},
            Parameters{
                {URL_PARAMS_CLIENT_ID,id},
                {URL_PARAMS_CATEGORY,MATH}
            },
            ConnectTimeout{CONNECTION_TIMEOUT},
            Timeout{config<int>(TIMEOUT)}
        );
        _say("Crawl response: ");
        _say(response.text);
        if (response.status_code == 0)
            markFailed(error,"Crawl request failed with no HTTP response");
        if (response.text.empty())
            markFailed(error,"Crawl returned empty response");
        OUTPUT(MATH_OUTPUT,MATH);
    }catch (std::exception &e) {
        testFinished = true;
        cppUtil::warn("Crashed !");
        POST_PARAMS_ID(localhost);
        throw e;
    }
    if (error)
        cppUtil::throwError("Test encountered an error !");
    else _say("Test Success !!! Now returning ...");
    testFinished = true;
    POST_PARAMS_ID(localhost);// To let main thread get out from listening port
}
