#include "config.h"

#ifdef DEVELOP
#define FORCE_GENERATE_CONFIG true
#elif
#define FORCE_GENERATE_CONFIG false
#endif

map<const string,std::any> defaultConfigs = map<const string,std::any>();

void createConfig(){
    char* path;
    defaultOutputChar(&path);
    if(getConfig(path,CONFIG_PATH)){
        dataStore::Data config = dataStore::Data::readFromJson(CONFIG_PATH,CONFIG_NAME);

        #if FORCE_GENERATE_CONFIG
        config = dataStore::Data{};
        config.setPath(CONFIG_PATH);
        config.setName(CONFIG_NAME);
        #endif

        config.put(VMID,"3493105986702255",false,FORCE_GENERATE_CONFIG);
        config.put(SUBSCRIBE_PUBLISH_TIME,3,false,FORCE_GENERATE_CONFIG);
        config.put(SUBSCRIBE_SEARCH_VIDEO_COUNT, 5, false, FORCE_GENERATE_CONFIG);
        config.put(WAIT_TIME,5000,false,FORCE_GENERATE_CONFIG);
        config.put(SEARCH_PAGE_SIZE, 50,false, FORCE_GENERATE_CONFIG);
        config.put(MAX_CRAWL_COUNT,10000,false,FORCE_GENERATE_CONFIG);
        config.put(MAX_AI_TOKENS,2000,false,FORCE_GENERATE_CONFIG);
        config.put(PORT,23223,false,FORCE_GENERATE_CONFIG);
        config.put(TIMEOUT,60000,false,FORCE_GENERATE_CONFIG);
        #ifdef DEVELOP
            config.put(DETAILS,false,false,FORCE_GENERATE_CONFIG);
        #else
            config.put(DETAILS,false,false,FORCE_GENERATE_CONFIG);
        #endif
        config.writeToJson();
    }
    freeOutputChar(&path);
}

void _readConfig() noexcept(false);

void readConfig() {
    char *path;
    defaultOutputChar(&path);
    toConfigPath(path, CONFIG_PATH);
    #ifdef DEVELOP
    if (FORCE_GENERATE_CONFIG) {
//        deleteConfig(CONFIG_PATH);
    #elif
    if (!fileExists(path) && createConfig(path, CONFIG_PATH)) {
    #endif
        createConfig();
        goto get;
    }
    freeOutputChar(&path);
    defaultOutputChar(&path);
    if (getConfig(path, CONFIG_PATH)) {
        get:
        try{
            _readConfig();
        }catch (exception e){
            defaultConfigs.clear();
            createConfig();
            try{
                _readConfig();
            }catch (exception e) {
                throwError("主程序配置文件错误，请删除文件重新生成！");
            }
        }
    }
    freeOutputChar(&path);
}

void _readConfig() noexcept(false){
    auto config = dataStore::Data::readFromJson(CONFIG_PATH, CONFIG_NAME);
    config.NeverSave();
    getAndStore<string>(&config, VMID);
    getAndStore<int>(&config, SUBSCRIBE_PUBLISH_TIME);
    getAndStore<int>(&config, SUBSCRIBE_SEARCH_VIDEO_COUNT);
    getAndStore<int>(&config, WAIT_TIME);
    getAndStore<int>(&config, SEARCH_PAGE_SIZE);
    getAndStore<int>(&config, MAX_CRAWL_COUNT);
    getAndStore<int>(&config, MAX_AI_TOKENS);
    getAndStore<int>(&config, PORT);
    getAndStore<int>(&config, TIMEOUT);
    getAndStore<bool>(&config, DETAILS);
}