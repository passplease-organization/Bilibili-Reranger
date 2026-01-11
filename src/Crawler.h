#pragma once
#include <atomic>
#include "pluginInterface.h"
#include "develop/flags.h"
#include "webAPIs/socialAPI.h"
#include "webAPIs/postgres.h"
#include <boost/asio.hpp>

using namespace std;

extern bool roughCheckVideo();
extern bool finalCheckVideo();
extern bool pluginDealJson(string&);
extern string pluginGetURL();

#define CLIENT_COOKIE (crawlInfo -> client -> handler() -> getCOOKIE().c_str())
extern webAPI::postgres dataBase;
extern string browseManagerUrl;
[[deprecated]] extern string user_agent;

bool crawl(const std::shared_ptr<const std::atomic<bool>>& cancel,boost::asio::ip::tcp::socket& socket);

[[deprecated]] string getURL(const crawlTask::Task* task = crawlTask::nowTask());

namespace webAPI {
    class BrowseWorker;
}
webAPI::BrowseWorker getWorker(const crawlTask::Task* task = crawlTask::nowTask());

bool checkEnv();
