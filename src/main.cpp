#include <iostream>
#include "Crawler.h"
#include "PluginHandler.h"
#include "pluginInterface.h"
#include "develop/flags.h"
#include "config.h"

using namespace std;

void setup(){
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

void clean(){
    curl_global_cleanup();
}

int main(){
    readConfig();
    PluginHandler::loadAll();

    if(!checkEnv()) {
        return 1;
    }

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
    if(crawl()) {
        cout << "运行成功，现在将退出程序！";
        clean();
        return 0;
    }
    cout << "运行失败，请检查具体原因！";
    clean();
    return 1;
}
