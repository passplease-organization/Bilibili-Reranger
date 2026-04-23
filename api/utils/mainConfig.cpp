#include "config.h"
#include "Util.h"

#ifdef DEVELOP
#define FORCE_GENERATE_CONFIG true
#else
#define FORCE_GENERATE_CONFIG false
#include "../webAPIs/socialAPI.h"
#endif

/**
 * Read only, shouldn't be modified
 */
Config defaultConfigs = toml::table();

#define _setConfigHelper(flag,value,description,recover) \
    cppUtil::setConfig(config,flag,value,description,recover)
#define _setConfig(...) _setConfigHelper(__VA_ARGS__,FORCE_GENERATE_CONFIG)

void readConfig() {
    char* path;
    defaultOutputChar(&path);
    toConfigPath(path, CONFIG_PATH);
    if(createConfig(path)){
        auto config = cppUtil::getConfig(path);

    #if FORCE_GENERATE_CONFIG
        config = toml::table();
    #endif

        _setConfig(SUBSCRIBE_PUBLISH_TIME,3,"关注博主视频计入爬取列表时间，单位：天");
    #ifdef WIN32
        _setConfig(WAIT_TIME,5000,"相邻两次爬取的间隔时间，单位毫秒");
    #elifdef __linux__
        _setConfig(WAIT_TIME,5000,"相邻两次爬取的间隔时间");
    #endif
        _setConfig(MAX_CRAWL_COUNT,25,"每次后台请求爬取次数上限");
        _setConfig(MAX_AI_TOKENS,20000,"每次AI请求最大token开销");
        _setConfig(PORT,23223,"监听端口");
        _setConfig(TIMEOUT,300000,"每个请求处理限时，单位毫秒");
    #ifdef DEVELOP
        _setConfig(DETAILS,true,"是否显示详细日志");
    #else
        _setConfig(DETAILS,false,"是否显示详细日志");
    #endif
        _setConfig(MAX_CLIENT,32,"后台支持的最大客户端数量");
        _setConfig(ADMIN_CLIENT_KEY,randomString(16),"管理员密码");
        _setConfig(POSTGRES_SSL_MODE,string("prefer"),"数据库连接方式");
    #ifndef DEVELOP
        _setConfig(POSTGRES_ENCRYPT_KEY,webAPI::SimpleESA::randomKey(),"数据写入数据库使用加密秘钥");
    #endif
        _setConfig(BROWSER_WORK_MAX_TIME,120,"Browser为每一次请求视频工作最大时长，单位：秒。若为负值则不限时");
        cppUtil::saveConfig(path,config);
        defaultConfigs = config;
    }
    freeOutputChar(&path);
}
