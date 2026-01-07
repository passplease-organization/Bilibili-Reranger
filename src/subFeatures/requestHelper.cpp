#include "requestHelper.h"

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

#define LOG(URL,NAME) \
    say("Accept URL: " URL);\
    say(NAME " working for that ...");

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
        data.put(URL_PARAMS_CATEGORY,group -> name,true);
    Json json = data;
    string payload = to_string(json);
    bool failed = payload.empty();
    return back(sendMessage(socket,payload,failed));
}

int login(boost::asio::ip::tcp::socket& socket) {
    LOG(LOGIN, "login");
    REQUIRE_CLIENT(socket);
#ifdef TEST
    string platform(BILIBILI);
#else
    string platform;
    for (auto const& param : crawlInfo -> params) {
        if (param.key == URL_PARAMS_PLATFORM){
            platform = param.value;
            break;
        }
    }
#endif
    if (platform.empty())
        return back(sendMessage(socket,webAPI::socialAPI::allPlatform(),false));
    crawlInfo -> client -> getHandler(platform,stop);
    if (!crawlInfo -> client -> check())
        return failed("Client Init failed !");
    auto const handler = getHandler(crawlInfo -> client);
#ifdef TEST
    string username,password;
#else
    string username = INFO_BODY(URL_PARAMS_USERNAME);
    string password = INFO_BODY(URL_PARAMS_PASSWORD);
#endif
    bool failed = false;
    string payload = handler -> login(username,password,failed);
    return back(sendMessage(socket,payload,failed));
}

int key(boost::asio::ip::tcp::socket& socket){
    LOG(KEY,"key");
    string key;
    if (BODY_CONTAIN(URL_PARAMS_ENCRYPT_KEY)) {
        key = INFO_BODY(URL_PARAMS_ENCRYPT_KEY);
        if (BODY_CONTAIN(URL_PARAMS_ADMIN)) {
            REQUIRE_CLIENT(socket);
            const string& admin = INFO_BODY(URL_PARAMS_ADMIN);
            const auto& id = webAPI::adminLogin(crawlInfo -> client -> getID(),admin);
            if (id == nullptr) {
                sendMessage(socket,"No this admin !",true);
                return failed();
            }
            const auto& esaKey = webAPI::client(id -> getID()) -> ESAKey(admin);
            Json json;
            json[URL_PARAMS_CLIENT_ID] = id -> getID();
            json[URL_PARAMS_ENCRYPT_KEY] = esaKey;
            return back(sendMessage(socket,crawlInfo -> client -> encrypt(json.dump())));
        }
    }
    if (key.empty()){
        Json json;
        json[URL_PARAMS_ENCRYPT_KEY] = webAPI::getRSA().publicKey();
        return back(sendMessage(socket, to_string(json),false));
    }
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
    if (crawlInfo -> checkClient())
        valid = true;
    return back(sendMessage(socket, valid ? "ID still valid !" : "ID not valid !", !valid));
}

int init(boost::asio::ip::tcp::socket& socket){
    LOG(INIT, "init");
    REQUIRE_CLIENT(socket);
    bool ok = crawlInfo -> client -> prepare();
    return back(sendMessage(socket, ok ? "准备过程完成" : "准备过程失败", !ok));
}

int set(boost::asio::ip::tcp::socket& socket){
    LOG(SET, "set");
    REQUIRE_CLIENT(socket);
    string platform;
    for (auto const& param : crawlInfo -> params) {
        if (param.key == URL_PARAMS_PLATFORM){
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
        sendMessage(socket,"Need Client Id !",true);
        return failed("Illegal Client");
    };
}
#endif
