#include "config.h"
#include "webAPIs/socialAPI.h"

#ifdef DEVELOP
#define FORCE_GENERATE_CONFIG true
#else
#define FORCE_GENERATE_CONFIG false
#endif

/**
 * Read only, shouldn't be modified
 */
map<const string,std::any> defaultConfigs = map<const string,std::any>();

extern string randomString(size_t length);

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
    #ifdef WIN32
        config.put(WAIT_TIME,5000,false,FORCE_GENERATE_CONFIG);
    #elifdef __linux__
        config.put(WAIT_TIME,2,false,FORCE_GENERATE_CONFIG);
    #endif
        config.put(SEARCH_PAGE_SIZE, 50,false, FORCE_GENERATE_CONFIG);
        config.put(MAX_CRAWL_COUNT,10000,false,FORCE_GENERATE_CONFIG);
        config.put(MAX_AI_TOKENS,2000,false,FORCE_GENERATE_CONFIG);
        config.put(PORT,23223,false,FORCE_GENERATE_CONFIG);
        config.put(TIMEOUT,60000,false,FORCE_GENERATE_CONFIG);
        #ifdef DEVELOP
            config.put(DETAILS,true,false,FORCE_GENERATE_CONFIG);
        #else
            config.put(DETAILS,false,false,FORCE_GENERATE_CONFIG);
        #endif
        config.put(KEY_LENGTH,2048,false,FORCE_GENERATE_CONFIG);
        config.put(MAX_CLIENT,32,false,FORCE_GENERATE_CONFIG);
        string adminKey = randomString(16);
        config.put(ADMIN_CLIENT_KEY,adminKey.c_str(),false,FORCE_GENERATE_CONFIG);
        config.put(POSTGRES_SSL_MODE,"prefer",false,FORCE_GENERATE_CONFIG);
        webAPI::SimpleESA esa(webAPI::SimpleESA::randomKey());
        config.put(POSTGRES_ENCRYPT_KEY,esa.getKey(adminKey).c_str(),false,FORCE_GENERATE_CONFIG);
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
    #else
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
    getAndStore<int>(&config, KEY_LENGTH);
    getAndStore<int>(&config, MAX_CLIENT);
    getAndStore<string>(&config, ADMIN_CLIENT_KEY);
    getAndStore<string>(&config, POSTGRES_SSL_MODE);
    getAndStore<string>(&config, POSTGRES_ENCRYPT_KEY);
}