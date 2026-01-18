#pragma once

#define videoByUser "https://app.biliapi.com/x/v2/space/archive/cursor"
#define mySubscribers "https://api.bilibili.com/x/relation/followings"
#define searchVideos "https://api.bilibili.com/x/web-interface/wbi/search/type?search_type=video"

#define checkResponse(json) json.contains("data")
#define COOKIE_WARN \
    warn("COOKIE可能不合法，请重新登录！"); \
    helper.clearData(); \
    helper.clearNextURL();
#define checkAndReturn(json) \
    if (!checkResponse(json)) { \
        COOKIE_WARN \
        return false; \
    }
#define containsData(json) json.contains("data")
#define getDataFromJson(json) json["data"]
#define _getSubscribers(json,all) (all ? getDataFromJson(json)["list"] : json["list"])
#define _getSubscriberCount(json) getDataFromJson(json)["total"].get<int>()
#define _getSubscriberName(json) json.value().at("uname").get<string>()
#define forEachVideo(json,label) for(const auto& videoData : getDataFromJson(json)[label])
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