#include "requestHelper.h"

#if NEED_PORT
#include "bilibiliAPIs.h"
#include "../PortListener.h"
#include "../exit.h"
#include "interface.h"
#include "../Crawler.h"
#include "../PluginHandler.h"

void dealParams(CurlHelper& helper) {
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

handler checkURL(const std::string& url) {
    if (url.starts_with(GET_ALL_CATEGORIES))
        return getAllCategories;
    else if (url.starts_with(SET_COOKIE))
        return setCOOKIE;
    return nullptr;
}
#endif