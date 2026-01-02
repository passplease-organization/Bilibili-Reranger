#include "requestHelper.h"

#if NEED_PORT
#include "bilibiliAPIs.h"
#include "../PortListener.h"
#include "../exit.h"
#include "interface.h"
#include "../Crawler.h"
#include "../PluginHandler.h"
#include "loginAPI/socialAPI.h"

namespace webAPI{
    socialAPI* getHandler(Client const* client){
        return client -> _handler;
    }
}

void dealParams(CrawlerHelper& helper) {
    if (!crawlInfo -> cookie.empty())
        helper.curlSetup(crawlInfo->cookie,user_agent);
}

int getAllCategories(boost::asio::ip::tcp::socket& socket) {
    say("Accept URL: " GET_ALL_CATEGORIES);
    say("getAllCategories working for that ...");
    if (stop -> load())
        return failed();
    auto& groups = crawlTask::getAllGroups();
    dataStore::Data data{};
    for (const auto group : groups)
        data.put(URL_PARAMS_CATEGORY,group -> name,true);
    Json json = data;
    return back(sendMessage(socket,to_string(json)));
}

int setCOOKIE(boost::asio::ip::tcp::socket& socket) {
    setEnv(crawlInfo -> cookie);
    const bool succeed = cookie == crawlInfo->cookie || (crawlInfo -> cookie.empty() && cookie == getenv(COOKIE));
    return back(sendMessage(socket, succeed ? "set cookie succeed" : "set cookie failed"));
}

int login(boost::asio::ip::tcp::socket& socket) {
    if (crawlInfo -> checkClient())
        return failed("Illegal client !");
    string platform;
    for (auto const& param : crawlInfo -> params) {
        if (param.key == URL_PARAMS_PLATFORM)
            platform = param.value;
    }
    string username = INFO_BODY(URL_PARAMS_USERNAME);
    string password = INFO_BODY(URL_PARAMS_PASSWORD);

    if (platform.empty())
        return back(sendMessage(socket,webAPI::socialAPI::allPlatform()));
    crawlInfo -> client -> getHandler(platform,stop);
    if (!crawlInfo -> client -> check())
        return failed("Client Init failed !");
    auto const handler = getHandler(crawlInfo -> client);
    return back(sendMessage(socket,handler -> login(username,password)));
}

int key(boost::asio::ip::tcp::socket& socket){
    string key;
    for (auto const& param : crawlInfo -> params) {
        if (param.key == URL_PARAMS_RSA_KEY)
            key = param.value;
    }
    if (key.empty()){
        Json json;
        json[URL_PARAMS_RSA_KEY] = webAPI::getRSA().publicKey();
        return sendMessage(socket, json);
    }
    auto& id = webAPI::createAndStoreClient(key);
    if (id.empty())
        return failed("Failed on setting client ID !");
    Json json;
    json[URL_PARAMS_CLIENT_ID] = id;
    return back(sendMessage(socket, webAPI::client(key) -> encrypt(json)));
}

int testID(boost::asio::ip::tcp::socket& socket) {
    bool valid = false;
    if (crawlInfo -> checkClient())
        valid = true;
    return back(sendMessage(socket, valid ? "ID still valid !" : "ID not valid !"));
}

handler checkURL(const std::string& url) {
    if (url.starts_with(GET_ALL_CATEGORIES))
        return getAllCategories;
    else if (url.starts_with(SET_COOKIE))
        return setCOOKIE;
    else if (url.starts_with(LOGIN))
        return login;
    else if (url.starts_with(KEY))
        return key;
    else if (url.starts_with(TEST_ID))
        return testID;
    return nullptr;
}
#endif