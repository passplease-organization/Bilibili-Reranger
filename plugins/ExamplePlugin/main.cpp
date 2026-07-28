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
    cppUtil::say({false, BLUE}, NAME);
    cppUtil::say({true, BLUE}, "插件开始加载");
    exampleConfig();
    char* _path;
    defaultOutputChar(&_path);
    toConfigPath(_path,CONFIG_PATH,MAX_BUFFER_SIZE,CONFIG_FILE_TYPE);
    if(createConfig(_path)) {
        cppUtil::say({false, nullptr},NAME "配置文件创建成功，路径：",_path);
        freeOutputChar(&_path);
        CONFIG = new dataStore::Data(dataStore::readFromJson(CONFIG_PATH,NAME));
        if(!CONFIG -> is_null()) {
        #if DEVELOP
        #else
            if (CONFIG -> empty()) {
                cppUtil::say("空配置文件，使用默认配置文件...");
                *CONFIG = getJson(EXAMPLE_NAME,EXAMPLE_PATH);
            }
        #endif
            return PluginStatus::SUCCESS;
        }
        cppUtil::warn({false, nullptr}, NAME " Open config failed !");
    }else{
        cppUtil::warn({false, nullptr}, NAME " Config file create failed ! Now path: ",_path);
        freeOutputChar(&_path);
    }
    return PluginStatus::FAIL;
}

void registerGroups(){
    if (CONFIG -> contains(GROUPS_LABEL)) {
        vector<dataStore::Data> array = (*CONFIG)[GROUPS_LABEL].get<vector<dataStore::Data>>();
        if(!array.empty()){
            for(auto& data : array){
                auto* group = new crawlTask::Group("",BILIBILI,0);
                crawlTask::group_from_data(data,group);
                crawlTask::registerGroup(group);
            }
            return;
        }
    }
    cppUtil::warn("空配置文件！请填写配置文件！文件：" CONFIG_PATH ".json");
}

VideoStatus roughJudge(const webAPI::Video& video){
    return VideoStatus::UNKNOWN;
}

VideoStatus judge(const webAPI::Video& video,unsigned short& score){
    score = 50;
    return VideoStatus::UNKNOWN;
}

#ifdef DEVELOP
const char* getURL(){
    cppUtil::say("Example Plugin Working For getURL ...");
    return "";
}

bool dealJson(const char* data){
    string tempData(data);
    cppUtil::say("Example Plugin Working For dealJson");
    return false;
}
#endif
