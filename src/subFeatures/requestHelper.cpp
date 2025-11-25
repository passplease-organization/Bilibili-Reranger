#include "requestHelper.h"

#if NEED_PORT
#include "../PortListener.h"
#include "../exit.h"
#include "interface.h"

int getAllCategories(boost::asio::ip::tcp::socket& socket) {
    say("Accept URL:" GET_ALL_CATEGORIES);
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

handler checkURL(const std::string& url) {
    if (url.starts_with(GET_ALL_CATEGORIES))
        return getAllCategories;
    return nullptr;
}
#endif