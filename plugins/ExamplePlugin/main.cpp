#include <iostream>
#include <vector>
#include "tasks.h"
#include "interface.h"

using namespace std;
using namespace crawlTask;

#if DEVELOP
#define CONFIG_PATH EXAMPLE_PATH
#else
#define CONFIG_PATH "Targets"
#endif
#define NAME "Example Plugin"

dataStore::Data* CONFIG = nullptr;

PluginStatus load(){
    say(NAME,false,BLUE);
    say("插件开始加载",true,BLUE);
    exampleConfig();
    char* _path;
    defaultOutputChar(&_path);
    if(getConfig(_path,CONFIG_PATH)) {
        say(NAME, false);
        say("配置文件创建成功，路径：",false);
        say(_path,true,BLUE);
        freeOutputChar(&_path);
        CONFIG = new dataStore::Data(dataStore::Data::readFromJson(CONFIG_PATH,NAME));
        if(CONFIG -> valid()) {
            CONFIG -> NeverSave();
            return PluginStatus::SUCCESS;
        }
        warn(NAME,false);
        warn("Open config failed !");
    }else{
        warn(NAME,false);
        warn(" Config file create failed ! Now path: ",false);
        warn(_path);
        freeOutputChar(&_path);
    }
    return PluginStatus::FAIL;
}

void registerGroups(){
    if(contains<const string,vector<dataStore::Data>>(GROUPS_LABEL,CONFIG -> dataArrays)){
        for(auto& data : CONFIG -> dataArrays[GROUPS_LABEL]){
            auto* group = new crawlTask::Group("",0);
            crawlTask::group_from_data(data,group);
            crawlTask::registerGroup(group);
        }
    }
}

VideoStatus roughJudge(){
    return VideoStatus::UNKNOWN;
}

VideoStatus judge(){
    return VideoStatus::UNKNOWN;
}

#ifdef DEVELOP
const char* getURL(){
    say("Example Plugin Working For getURL ...");
    return "";
}

bool dealJson(const char* data){
    string tempData(data);
    say("Example Plugin Working For dealJson");
    return false;
}
#endif