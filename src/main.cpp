#include <iostream>
#include <utility>
#include <chrono>
#include "Crawler.h"
#include "PluginHandler.h"
#include "pluginInterface.h"
#include "develop/flags.h"
#include "config.h"
#if NEED_PORT
    #include "PortListener.h"
#else
    #include <atomic>
#endif

inline void setup(){
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

inline void clean(){
    curl_global_cleanup();
}

#if NEED_PORT
int work(const string& target,const map<const string,std::any>& config,const std::atomic<bool>& cancel){
    defaultConfigs = config;
#else
int main(int argc, char** argv){
    string target;
    readConfig();
    PluginHandler::loadAll();
    std::atomic cancel(false);
#endif

#ifdef WIN32
    warn("这个程序是为Linux系统设计的，对于Windows系统难保可用性，不建议在Windows上使用！");
#endif

    if(!checkEnv()) {
        return 1;
    }

    crawlTask::GroupFilter(target);
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
    if(crawl(cancel)) {
        #if NEED_PORT
            say("运行成功，本进程即将结束");
        #else
            say("运行成功，现在将退出程序！");
        #endif
        clean();
        return 0;
    }
    warn("运行失败，请检查具体原因！");
    clean();
    return 1;
}
#if NEED_PORT
int main(int argc, char** argv) {
    startWork();
}
#endif