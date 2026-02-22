#pragma once

#include "develop/flags.h"

#define deprecated_videoByUser "https://app.biliapi.com/x/v2/space/archive/cursor"
#define deprecated_mySubscribers "https://api.bilibili.com/x/relation/followings"
#define deprecated_searchVideos "https://api.bilibili.com/x/web-interface/wbi/search/type?search_type=video"

#define containsData(json) json.contains("data")
#define COOKIE_WARN \
    warn("COOKIE可能不合法，请重新登录！"); \
    helper.clearData(); \
    helper.clearNextURL();
#define checkAndReturn(json) \
    if (!containsData(json)) { \
        COOKIE_WARN \
        warn("收到Json: ",false); \
        warn((json).dump().c_str()); \
        return false; \
    }
#define getDataFromJson(json) json["data"]
#define containsList(json) getDataFromJson(json).contains("list")
#define _getListFromData(json,all) (all ? getDataFromJson(json)["list"] : json["list"])
#define _getSubscriberCount(json) getDataFromJson(json)["total"].get<int>()
#define _getSubscriberName(json) json.at("uname").get<string>()
#define _getSubscriberMid(json) json.at("mid").get<string>()
#define _hasSubscriberName(json) json.contains("uname")
#define _hasSubscriberMid(json) json.contains("mid")
#define forEachVideo(json) for(const auto& videoData : _getListFromData(json,true)["vList"])
#define ofPerson "item"
#define ofSearch "result"
#define checkResult(json) \
    if (!getDataFromJson(json).contains(ofSearch)) { \
        COOKIE_WARN \
        return false; \
    }

// Environments:
#define COOKIE "COOKIE"
#define USERAGENT "USERAGENT"
#if NEED_PORT
    #define POSTGRES_SCHEMA "DB_NAME"
    #define POSTGRES_USER "DB_USER"
    #define POSTGRES_PASSWORD "DB_PASSWORD"
    #define POSTGRES_HOST "DB_HOST"
    #define POSTGRES_DELETE "DB_DELETE"
    #define POSTGRES_PORT "DB_PORT"
    #define BROWSE_MANAGER_URL "BROWSE_URL"
#endif