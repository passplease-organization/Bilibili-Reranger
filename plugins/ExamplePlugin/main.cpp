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
        #if DEVELOP
        #else
            if (CONFIG -> empty()) {
                say("空配置文件，使用默认配置文件...");
                *CONFIG = getJson(EXAMPLE_NAME,EXAMPLE_PATH);
            }
        #endif
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
    if (CONFIG -> contains(GROUPS_LABEL)) {
        vector<dataStore::Data> array;
        CONFIG -> get(GROUPS_LABEL,&array);
        if(!array.empty()){
            for(auto& data : array){
                auto* group = new crawlTask::Group("",BILIBILI,0);
                crawlTask::group_from_data(data,group);
                crawlTask::registerGroup(group);
            }
            return;
        }
    }
    warn("空配置文件！请填写配置文件！文件：" CONFIG_PATH ".json");
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
