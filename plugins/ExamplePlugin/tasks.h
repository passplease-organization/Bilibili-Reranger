#pragma once

#include "utils/Util.h"
#include "utils/configUtil.h"
#include "pluginInterface.h"
#include "webAPIs/platforms.h"

crawlTask::Group political("political",BILIBILI,0);
crawlTask::Group math("math",BILIBILI,0);
crawlTask::Group physical("physical",BILIBILI,0);
crawlTask::Group chemistry("chemistry",BILIBILI,0);
crawlTask::Group biological("biological",BILIBILI,0);

#define EXAMPLE_PATH "ExampleConfig"
#define EXAMPLE_NAME "example_for_example_plugin"
#define CONFIG_FILE_TYPE ".json"
#define GROUPS_LABEL "groups"

#ifdef DEVELOP
    #define FORCE_CONFIG true
#else
    #define FORCE_CONFIG false
#endif

void initGroups(){
    crawlTask::Task* a = nullptr;
    a = new crawlTask::Task{"波士顿圆脸",10,crawlTask::WorkingMode::SUBSCRIBE};
    political.registerTask(a);
    a = new crawlTask::Task("军情D7处",10,crawlTask::WorkingMode::SUBSCRIBE);
    political.registerTask(a);
    a = new crawlTask::Task("巴以冲突",20);
    political.registerTask(a);

    a = new crawlTask::Task("选择公理",5);
    math.registerTask(a);
    a = new crawlTask::Task("数学",20,crawlTask::WorkingMode::TAG);
    math.registerTask(a);

    a = new crawlTask::Task("物理",10);
    physical.registerTask(a);

    a = new crawlTask::Task("有机化学",5);
    chemistry.registerTask(a);
    a = new crawlTask::Task("化学",5,crawlTask::WorkingMode::TAG);
    chemistry.registerTask(a);

    a = new crawlTask::Task("生物",20);
    biological.registerTask(a);
}

void exampleConfig(){
    char* path;
    defaultOutputChar(&path);
    toConfigPath(path,EXAMPLE_PATH);
    #if FORCE_CONFIG
        deleteConfig(path);
    #endif
    if(createConfig(path)) {
        initGroups();
        auto example = dataStore::Data::readFromJson(EXAMPLE_PATH,EXAMPLE_NAME);
        dataStore::Data a{};
        crawlTask::group_to_data(a,&political);
        dataStore::Data* b = nullptr;
        a.copy(&b);
        example.put(GROUPS_LABEL,b,true);
        a.clear();

        crawlTask::group_to_data(a,&math);
        a.copy(&b);
        example.put(GROUPS_LABEL,b, true);
        a.clear();

        crawlTask::group_to_data(a,&physical);
        a.copy(&b);
        example.put(GROUPS_LABEL,b, true);
        a.clear();

        crawlTask::group_to_data(a,&chemistry);
        a.copy(&b);
        example.put(GROUPS_LABEL,b, true);
        a.clear();

        crawlTask::group_to_data(a,&biological);
        a.copy(&b);
        example.put(GROUPS_LABEL,b, true);
        a.clear();

        example.writeToJson();
    }
    freeOutputChar(&path);
}
