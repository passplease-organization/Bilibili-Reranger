#include "testCode.h"

#ifdef TEST
#include "utils/Util.h"
#include "../../api/utils/config.h"
#include <cpr/cpr.h>
#include <thread>
#include <csignal>
#include <unistd.h>
#include "../subFeatures/requestHelper.h"
#include "utils/BilibiliInterface.h"
#include "webAPIs/socialAPI.h"
#include "webAPIs/platforms.h"
#include "utils/schedules.h"
#include "PortListener.h"

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
        deleteConfig(output); \
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
#define SCHEDULE_OUTPUT OUTPUT_DIRECTORY "/scheduled_crawl.json"
#define DEBUG "test"

namespace {
    constexpr int SESSION_POLL_INTERVAL_MS = 5000;
    constexpr int SESSION_POLL_RETRY_COUNT = 100;

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

    void expectStatusCodeValue(const long actual, const long expected, bool& error, const string& step) {
        if (actual == expected)
            return;
        error = true;
        cppUtil::warn({false, nullptr}, step);
        cppUtil::warn(" failed, status code: ");
        cppUtil::warn(actual);
    }

    string extractSession(const Response& response, webAPI::SimpleESA* esa, bool& error, const string& step) {
        expectStatusCode(response, 200, error, step + " session response");
        const string decrypted = decryptIfNeeded(response.text, esa);
        if (decrypted.empty()) {
            markFailed(error, step + " session response is empty");
            return "";
        }
        const Json json = parseJsonChecked(decrypted, error, step + " session response");
        if (!json.contains(URL_PARAMS_SESSION) || !json[URL_PARAMS_SESSION].is_string()) {
            markFailed(error, step + " session response missing session");
            return "";
        }
        return json[URL_PARAMS_SESSION];
    }

    Json waitSessionResult(
        const string& localhost,
        const string& id,
        const string& session,
        webAPI::SimpleESA* esa,
        bool& error,
        const string& step,
        long* statusCode = nullptr
    ) {
        if (session.empty()) {
            markFailed(error, step + " session is empty");
            return Json();
        }
        Response response;
        Json json;
        bool finished = false;
        for (int retry = 0; retry < SESSION_POLL_RETRY_COUNT; retry++) {
            response = Post(
                Url{localhost + GET},
                Parameters{
                    {URL_PARAMS_CLIENT_ID, id},
                    {URL_PARAMS_SESSION, session}
                },
                ConnectTimeout{CONNECTION_TIMEOUT},
                Timeout{config<int>(TIMEOUT)}
            );
            const string decrypted = decryptIfNeeded(response.text, esa);
            json = parseJsonChecked(decrypted, error, step + " poll");
            if (json.contains(SESSION_FINISHED) && json[SESSION_FINISHED].is_boolean()) {
                finished = json[SESSION_FINISHED];
                if (finished)
                    break;
            }
            this_thread::sleep_for(chrono::milliseconds(SESSION_POLL_INTERVAL_MS));
        }
        if (statusCode != nullptr)
            *statusCode = response.status_code;
        if (!finished)
            markFailed(error, step + " session did not finish in time");
        if (!json.contains(SESSION_OK) || !json[SESSION_OK].is_boolean())
            markFailed(error, step + " poll response missing ok");
        if (!json.contains(SESSION_DATA))
            markFailed(error, step + " poll response missing data");
        return json;
    }

    string stringifySessionData(const Json& json, bool& error, const string& step) {
        if (!json.contains(SESSION_DATA)) {
            markFailed(error, step + " poll response missing data");
            return "";
        }
        const auto& data = json[SESSION_DATA];
        if (data.is_string())
            return data.get<string>();
        return data.dump();
    }
}

void test() {
    blockSignal();
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

        {
            const Json videoJson = Json::parse(R"json({"av_feature":null,"business_info":null,"bvid":"BV1kaoEBhEpo","cid":37790092896,"dislike_switch":1,"dislike_switch_pc":1,"duration":2982,"enable_vt":0,"goto":"av","id":116460286645708,"is_followed":0,"is_stock":0,"ogv_info":null,"owner":{"face":"https://i0.hdslb.com/bfs/face/9a06bde169d709c448e4927376f6c0483a9b7dce.jpg","mid":3493130242362171,"name":"Minecraft辰天"},"pic":"http://i2.hdslb.com/bfs/archive/21030591bd5e91a2849c52fb031ddd013c1ce957.jpg","pic_4_3":"http://i0.hdslb.com/bfs/archive/7144a92d1d9f84c2b720608726d1dea17f0e3e2c.jpg","pos":0,"pubdate":1777043498,"rcmd_reason":{"reason_type":0},"room_info":null,"show_info":1,"stat":{"danmaku":837,"like":1663,"view":53679,"vt":0},"title":"【我的世界】挑战压缩木剑生存一口气带你看完！","track_id":"web_pegasus_0.router-web-pegasus-2479516-f6fct.1779002738942.483","uri":"https://www.bilibili.com/video/BV1kaoEBhEpo","vt_display":""})json");
            const auto video = webAPI::Video::fromJson(videoJson);
            if (string(video.author()) != "Minecraft辰天")
                markFailed(error,"Video normalization failed: author");
            if (string(video.description()) != "")
                markFailed(error,"Video normalization failed: description");
            if (video.mid() != 3493130242362171LL)
                markFailed(error,"Video normalization failed: mid");
            if (string(video.duration()) != "49:42")
                markFailed(error,"Video normalization failed: duration");
            if (video.views() != 53679U)
                markFailed(error,"Video normalization failed: views");
            if (video.popups() != 837U)
                markFailed(error,"Video normalization failed: popups");
            if (string(video.url()) != "https://www.bilibili.com/video/BV1kaoEBhEpo")
                markFailed(error,"Video normalization failed: url");
        }

        {
            const Json videoJson = Json::parse(R"json({"author":"和谐号舰长","description":"","popups":213,"publishTime":"2026-05-01","title":"新版本的物理系统有多逆天？什么叫把一切生物当球踢？","url":"https://www.bilibili.com/video/BV1G69UBZEZh","videoTime":"4:4","videoURL":"https://i1.hdslb.com/bfs/archive/c796ed3a3c570f6667912f1de30de5954b61bf7f.jpg","views":98239})json");
            Json feedbackJson;
            feedbackJson[BODY_PARAMS_PLATFORM] = BILIBILI;
            feedbackJson[BODY_PARAMS_SCORE] = 3;
            feedbackJson[BODY_PARAMS_VIDEO] = videoJson;
            const webAPI::formater::FeedBack feedback(feedbackJson);
            if (feedback.score != 3)
                markFailed(error,"Feedback json failed: score");
            if (string(feedback.video.author()) != "和谐号舰长")
                markFailed(error,"Feedback video json failed: author");
            if (string(feedback.video.description()) != "")
                markFailed(error,"Feedback video json failed: description");
            if (string(feedback.video.string_PublishTime()) != "2026-05-01")
                markFailed(error,"Feedback video json failed: publishTime");
            if (string(feedback.video.duration()) != "4:4")
                markFailed(error,"Feedback video json failed: videoTime");
            if (feedback.video.views() != 98239U)
                markFailed(error,"Feedback video json failed: views");
            if (feedback.video.popups() != 213U)
                markFailed(error,"Feedback video json failed: popups");
            if (string(feedback.video.url()) != "https://www.bilibili.com/video/BV1G69UBZEZh")
                markFailed(error,"Feedback video json failed: url");

            const Json serializedVideo = feedback.video;
            if (serializedVideo["videoURL"] != videoJson["videoURL"])
                markFailed(error,"Feedback video json serialization failed: videoURL");
        }

        {
            // null numeric fields must be normalized to 0 instead of throwing
            const Json rawJson = Json::parse(R"json({"author":"测试作者","bvid":"BV1kaoEBhEpo","description":"测试","duration":2982,"owner":{"mid":123,"name":"测试作者"},"pic":"https://example.com/pic.jpg","pubdate":1777043498,"stat":{"danmaku":null,"view":null},"title":"测试视频","play":null,"video_review":null})json");
            const auto video = webAPI::Video::fromJson(rawJson);
            if (video.views() != 0U)
                markFailed(error,"Video null views fallback failed");
            if (video.popups() != 0U)
                markFailed(error,"Video null popups fallback failed");
            if (video.mid() != 123LL)
                markFailed(error,"Video mid normalization failed");
            if (video.getJson()["play"] != 0)
                markFailed(error,"Video json play not normalized to 0");

            const Json feedbackJson = Json::parse(R"json({"author":"测试作者","description":"测试","popups":null,"publishTime":null,"title":"测试视频","url":"https://www.bilibili.com/video/BV1kaoEBhEpo","videoTime":"4:4","videoURL":"https://example.com/pic.jpg","views":null})json");
            const auto feedbackVideo = webAPI::Video::fromCompatibleJson(feedbackJson);
            if (feedbackVideo.views() != 0U)
                markFailed(error,"Feedback video null views fallback failed");
            if (feedbackVideo.popups() != 0U)
                markFailed(error,"Feedback video null popups fallback failed");
            const Json serializedVideo = feedbackVideo;
            if (serializedVideo["views"] != 0)
                markFailed(error,"Feedback video json views not normalized to 0");
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
        {
            const string session = extractSession(response,&esa,error,"Login status");
            long statusCode = 0;
            json = waitSessionResult(localhost,id,session,&esa,error,"Login status",&statusCode);
            const string decrypted = stringifySessionData(json,error,"Login status");
            _say("Login status: ");
            _say(decrypted);
            if (statusCode != 200 && statusCode != 500)
                markFailed(error,"Login status returned unexpected status code");
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
        {
            const string session = extractSession(response,&esa,error,"Login");
            long statusCode = 0;
            json = waitSessionResult(localhost,id,session,&esa,error,"Login",&statusCode);
            const Json sessionData = json.contains(SESSION_DATA) ? json[SESSION_DATA] : Json();
            const string decrypted = sessionData.is_string() ? sessionData.get<string>() : sessionData.dump();
            _say("Login: ");
            _say(decrypted);
#if ALL_CONTAINER_ONLINE
            if (statusCode != 200 && statusCode != 500)
                markFailed(error,"Login returned unexpected status code");
            if (decrypted.empty()) {
                error = true;
                EMPTY_WARN("Login");
            }
            if (!sessionData.is_object())
                markFailed(error,"Login data should be json object");
            if (!sessionData.contains("success") || !sessionData["success"].is_boolean())
                markFailed(error,"Login missing boolean success");
            if (!sessionData.contains("url") || !sessionData["url"].is_string())
                markFailed(error,"Login missing string url");
            if (sessionData.value("success", false) && sessionData.value("url", string()).empty())
                markFailed(error,"Login success is true but url is empty");
            json[DEBUG] = decrypted;
#else
            expectStatusCodeValue(statusCode,200,error,"Login");
            expectTextEquals(decrypted,"测试成功",error,"Offline login");
            json.clear();
            json[DEBUG] = decrypted;
#endif
        }
        OUTPUT(LOGIN_OUTPUT,LOGIN_NO_SLASH);

        _say("Init:");
        response = POST_PARAMS_ID(localhost + INIT);
        {
            const string session = extractSession(response,&esa,error,"Init");
            long statusCode = 0;
            json = waitSessionResult(localhost,id,session,&esa,error,"Init",&statusCode);
            const string decrypted = stringifySessionData(json,error,"Init");
            _say("Init: ");
            _say(decrypted);
#if ALL_CONTAINER_ONLINE
            if (statusCode != 200 && statusCode != 500)
                markFailed(error,"Init returned unexpected status code");
            if (decrypted != "准备过程完成" && decrypted != "准备过程失败")
                markFailed(error,"Init returned unexpected text");
#else
            expectStatusCodeValue(statusCode,200,error,"Init");
            expectTextEquals(decrypted,"准备过程完成",error,"Offline init");
#endif
            json.clear();
            json[DEBUG] = decrypted;
            OUTPUT(INIT_OUTPUT,INIT_NO_SLASH);
        }

        _say("\nTest exit listener:");
        // 模拟 docker stop / kill 默认行为：向本进程发送 SIGTERM(15)。
        // 说明：多线程进程中，进程级信号会投递给任意未阻塞该信号的线程。
        // 若投递到测试/调度/工作线程(它们没有处理器)，默认处置会直接终止进程，
        // 退出监听即失败；只有投递到监听线程，sigwait 才能拦截并执行收尾。
        // 若本次测试通过，下方日志应出现：
        //   "收到退出信号，即将退出程序，信号15" (stopWork)
        //   "收到退出请求，进行退出" + "退出定时工作线程" (schedule 线程退出)
        kill(getpid(), SIGTERM);
        if (!waitForExitCleanup(15000))
            markFailed(error,"Exit listener did not finish cleanup in time");
        else
            _say("退出监听器已处理退出信号并停止调度线程");
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
#endif
