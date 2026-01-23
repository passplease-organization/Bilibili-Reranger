#pragma once
#include <atomic>
#include "pluginInterface.h"
#include "develop/flags.h"
#include "webAPIs/socialAPI.h"
#if NEED_PORT
    #include <boost/asio.hpp>
#endif

using namespace std;

extern bool roughCheckVideo();
extern bool finalCheckVideo();
extern bool pluginDealJson(string&);
extern string pluginGetURL();

#if NEED_PORT
    #define CLIENT_COOKIE (crawlInfo -> client -> handler() -> getCOOKIE().c_str())
    namespace socialAPI {
        class postgres;
    }
    extern socialAPI::postgres dataBase;
#else
    extern string cookie;
#endif
extern string user_agent;

#if NEED_PORT
bool crawl(const std::atomic<bool>& cancel,boost::asio::ip::tcp::socket& socket);
#else
bool crawl(const std::atomic<bool>& cancel);
#endif

string getURL(const crawlTask::Task* task = crawlTask::nowTask());

bool checkEnv();