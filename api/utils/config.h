#pragma once

#include <string>
#include <toml.hpp>
#include "../APIStatus.h"
#include "configUtil.h"

#define CONFIG_PATH "mainConfig"
#define CONFIG_NAME "MainConfig"
#define STR_IMPL(x) #x
#define STR(x) STR_IMPL(x)
#define NAME BiliBili_Reranger
#define NAME_STR STR(NAME)
#define VERSION 3.1
#define VERSION_STR STR(VERSION)

#define SUBSCRIBE_PUBLISH_TIME "subscriber_publish_time"
#define WAIT_TIME "pause_time_between_crawls"
#define MAX_CRAWL_COUNT "max_crawl_count_per_work"
#define MAX_AI_TOKENS "max_tokens"
#define PORT "port"
#define TIMEOUT "timeout"
#define DETAILS "debug"
#define MAX_CLIENT "max_client_count"
#define ADMIN_CLIENT_KEY "admin_client_key"
#define POSTGRES_SSL_MODE "postgres_connection_mode"
#define SCHEDULE_CRAWL_INTERNAL "schedule_crawl_internal"
#ifndef DEVELOP
    #define POSTGRES_ENCRYPT_KEY "postgres_encrypt_key"
#endif
#define BROWSER_WORK_MAX_TIME "browser_timeout"
#define COOKIE_STORE_CHECK "cookie_check"

extern API Config defaultConfigs;

API void readConfig();

template<typename T>
inline T config(const char* label) noexcept(false){
    static_assert(cppUtil::SupportedValue<T> || cppUtil::ConvirtableValue<T>,"Unsupported config type");
    if constexpr (cppUtil::SupportedValue<T>)
        return toml::find<T>(defaultConfigs,label);
    else {
        T t;
        return from_toml(defaultConfigs,t);
    }
}