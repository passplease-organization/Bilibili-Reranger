#pragma once

#include "Util.h"

#define CONFIG_PATH "mainConfig"
#define CONFIG_NAME "MainConfig"

#define VMID "mid"
#define SUBSCRIBE_PUBLISH_TIME "subscriber_publish_time"
#define SUBSCRIBE_SEARCH_VIDEO_COUNT "subscribe_search_video_count"
#define SEARCH_PAGE_SIZE "search_page_size"
#define WAIT_TIME "pause_time_between_crawls"
#define MAX_CRAWL_COUNT "max_crawl_count_per_work"
#define MAX_AI_TOKENS "max_tokens"
#define PORT "port"
#define TIMEOUT "timeout"

extern API map<const string,std::any> defaultConfigs;

API void createConfig();

API void readConfig();

template<typename T>
inline void storeConfig(const char* label,T* t){
    defaultConfigs[label] = *t;
}

template<typename T>
inline void getAndStore(dataStore::Data* data, const char* label){
    T* t;
    dataStore::getMap<T>(data,label,&t);
    storeConfig(label,t);
}

template<typename T>
T& config(const char* label) noexcept(false){
    return std::any_cast<T&>(defaultConfigs.at(label));
}
