#include "requestHelper.h"

#include "config.h"

#if NEED_PORT

#ifdef TEST
    #include "BilibiliInterface.h"
#endif
#include <iostream>

#include "../PortListener.h"
#include "../exit.h"
#include "interface.h"
#include "../Crawler.h"
#include "../PluginHandler.h"
#include "loginAPI/socialAPI.h"
#include "loginAPI/platforms.h"

#define LOG(URL,NAME) \
    say("Accept URL: " URL);\
    say(NAME " working for that ...");
#define RELEASE_LOG(message) \
    if (config<bool>(DETAILS)) \
        say(message)

namespace webAPI{
    socialAPI* getHandler(Client const* client){
        return client -> _handler;
    }
}

void dealParams(CrawlerHelper& helper) {}

int getAllCategories(boost::asio::ip::tcp::socket& socket) {
    LOG(GET_ALL_CATEGORIES,"getAllCategories");
    if (stop -> load())
        return failed();
    auto& groups = crawlTask::getAllGroups();
    dataStore::Data data{};
    for (const auto group : groups)
        data.put(group -> platform,group -> name,true);
    const Json json = data;
    const string payload = to_string(json);
    return back(sendMessage(socket,payload,payload.empty()));
}

int login(boost::asio::ip::tcp::socket& socket) {
    LOG(LOGIN, "login");
    REQUIRE_CLIENT(socket);
#ifdef TEST
    string platform(BILIBILI),test("");
#else
    string test;
    for (auto const& param : crawlInfo -> params) {
        if (param.key == URL_PARAMS_TEST) {
            test = param.value;
            break;
        }
    }
#endif
    if (test == "true") {
        RELEASE_LOG("测试后台登录状态");
        if (crawlInfo -> client -> handler() != nullptr) {
            if (crawlInfo -> client -> handler() -> validCOOKIE())
                return back(sendMessage(socket,"有效COOKIE"));
            return back(sendMessage(socket,"无效COOKIE",true));
        }
        return back(sendMessage(socket,"未设置平台！",true));
    }
    if (!BODY_CONTAIN(BODY_PARAMS_PLATFORM)) {
        RELEASE_LOG("获取全部平台");
        return back(sendMessage(socket,webAPI::socialAPI::allPlatform(),false));
    }
    RELEASE_LOG("登录操作");
    crawlInfo -> client -> getHandler(INFO_BODY(BODY_PARAMS_PLATFORM),stop);
    if (!crawlInfo -> client -> check())
        return failed("Client Init failed !");
    auto const handler = getHandler(crawlInfo -> client);
#ifdef TEST
    string username,password;
#else
    string username = BODY_CONTAIN(URL_PARAMS_USERNAME) ? INFO_BODY(URL_PARAMS_USERNAME) : "";
    string password = BODY_CONTAIN(URL_PARAMS_PASSWORD) ? INFO_BODY(URL_PARAMS_PASSWORD) : "";
#endif
    bool failed = false;
    string payload = handler -> login(username,password,failed);
    return back(sendMessage(socket,payload,failed));
}

int key(boost::asio::ip::tcp::socket& socket){
    LOG(KEY,"key");
    if (BODY_CONTAIN(BODY_PARAMS_ADMIN)) {
        RELEASE_LOG("管理员登录");
        REQUIRE_CLIENT(socket);
        const string& admin = INFO_BODY(BODY_PARAMS_ADMIN);
        const auto& c = webAPI::adminLogin(crawlInfo -> client -> getID(),admin);
        if (c == nullptr) {
            sendMessage(socket,"No this admin !",true);
            return failed();
        }
        const auto& esaKey = webAPI::client(c -> getID()) -> ESAKey(admin);
        Json json;
        json[URL_PARAMS_CLIENT_ID] = c -> getID();
        json[BODY_PARAMS_ENCRYPT_KEY] = esaKey;
        #if TEST
            say("ESA key: ",false);
            say(c -> ESAKey("test").c_str());
        #endif
        return back(sendMessage(socket,json.dump()));
    }
    string key;
    if (BODY_CONTAIN(BODY_PARAMS_ENCRYPT_KEY)) {
        key = INFO_BODY(BODY_PARAMS_ENCRYPT_KEY);
    }
    if (key.empty()){
        RELEASE_LOG("获取公钥");
        Json json;
        json[BODY_PARAMS_ENCRYPT_KEY] = webAPI::getRSA().publicKey();
        return back(sendMessage(socket, to_string(json),false));
    }
    RELEASE_LOG("注册新客户端");
    const auto& id = webAPI::createAndStoreClient(key);
    if (id.empty())
        return failed("Failed on setting client ID !");
    Json json;
    json[URL_PARAMS_CLIENT_ID] = id;
    return back(sendMessage(socket, webAPI::client(id) -> encrypt(json.dump()),false));
}

int testID(boost::asio::ip::tcp::socket& socket) {
    LOG(TEST_ID,"testID");
    bool valid = false;
    if (crawlInfo -> checkClient()) {
        valid = true;
    }
    string body = valid ? "ID still valid !" : "ID not valid !";
    if (valid && BODY_CONTAIN(BODY_PARAMS_ENCRYPT_KEY)) {
        RELEASE_LOG("检查对称加密秘钥");
        valid = crawlInfo -> client -> isKey(INFO_BODY(BODY_PARAMS_ENCRYPT_KEY));
        body = valid ? "correct key" : "wrong key";
    }
    return back(sendMessage(socket, body, !valid));
}

int init(boost::asio::ip::tcp::socket& socket){
    LOG(INIT, "init");
    REQUIRE_CLIENT(socket);
    const bool& ok = crawlInfo -> client -> prepare();
    return back(sendMessage(socket, ok ? "准备过程完成" : "准备过程失败", !ok));
}

int set(boost::asio::ip::tcp::socket& socket){
    LOG(SET, "set");
    REQUIRE_CLIENT(socket);
    string platform;
    for (auto const& param : crawlInfo -> params) {
        if (param.key == BODY_PARAMS_PLATFORM){
            platform = param.value;
            break;
        }
    }
    if (!platform.empty())
        crawlInfo -> client -> getHandler(platform,stop);
    return back(sendMessage(socket,"设置成功"));
}

handler checkURL(const std::string& url) {
    if (url.starts_with(GET_ALL_CATEGORIES))
        return getAllCategories;
    else if (url.starts_with(LOGIN))
        return login;
    else if (url.starts_with(KEY))
        return key;
    else if (url.starts_with(TEST_ID))
        return testID;
    else if (url.starts_with(INIT))
        return init;
    else if (url.starts_with(SET))
        return set;
    return nullptr;
}

handler requireClient(){
    if (crawlInfo -> checkClient())
        return nullptr;
    return [](boost::asio::ip::tcp::socket& socket) -> int{
        RELEASE_LOG("无客户端ID请求！");
        sendMessage(socket,"Need Client Id !",true);
        return failed("Illegal Client");
    };
}
#endif
