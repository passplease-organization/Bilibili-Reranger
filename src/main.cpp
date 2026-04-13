#include <iostream>
#include <chrono>

#include "Crawler.h"
#include "PluginHandler.h"
#include "pluginInterface.h"
#include "develop/flags.h"
#include "config.h"
#include "exit.h"
#include "platforms/bilibiliHandler.h"
#include "PortListener.h"
#include "subFeatures/requestHelper.h"
#include <boost/asio.hpp>

inline void setup(){
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

void clean(){
    curl_global_cleanup();
}

int work(const CrawlInfo info,shared_ptr<const atomic<bool>> cancel,boost::asio::ip::tcp::socket socket){
    setThreadId(info.id);
    crawlInfo = &info;
    stop = cancel;
    if (crawlInfo -> client != nullptr && crawlInfo -> client -> handler() != nullptr) {
        crawlInfo -> client -> resetTimer(stop);
    }

#ifdef WIN32
    warn("这个程序是为Linux系统设计的，对于Windows系统难保可用性，不建议在Windows上使用！");
#endif

    #ifdef TEST
        GroupFilter(info.target,crawlInfo -> client != nullptr && crawlInfo -> client -> handler() != nullptr ? crawlInfo -> client -> handler() -> support() : ALL_PLATFORMS);
    #else
        if (crawlInfo -> client != nullptr && crawlInfo -> client -> handler() != nullptr)
            GroupFilter(info.target,crawlInfo -> client -> handler() -> support());
    #endif
    PluginHandler::forEachPlugin([](PluginHandler& plugin) -> PluginStatus {
        return plugin.registerGroups();
    });

#if TEST_DLL
    auto task = crawlTask::nowTask();
    say("第一个注册的任务：",false);
    say(task -> keyword,true,GREEN);
    say("其工作状态：",false);
    say(crawlTask::getName(task -> mode),true,GREEN);
    cout << "当前处于测试插件状态，主程序已退出" << endl;
    return 0;
#endif

    setup();
    if (const auto& handler = checkURL(info.url); handler != nullptr) {
        if (const int& signal = handler(socket); signal != NEED_NORMAL_HANDLE)
            return signal;
    }
    if (config<bool>(DETAILS))
        say("爬取工作开始");
    REQUIRE_CLIENT(socket);
    try{
        if(crawl(cancel,socket)) {
            return success();
        }
    }catch(const std::exception& e) {
        warn("crawl encounter exception: ",false);
        warn(e.what());
    }
    return failed();
}
int main(int argc, char** argv) {
    return startWork();
}
