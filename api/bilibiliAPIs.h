#pragma once

#define videoByUser "https://app.biliapi.com/x/v2/space/archive/cursor"
#define mySubscribers "https://api.bilibili.com/x/relation/followings"
#define searchVideos "https://api.bilibili.com/x/web-interface/wbi/search/type?search_type=video"

#define getDataFromJson(json) json["data"]
#define getSubscribers(json,all) (all ? getDataFromJson(json)["list"] : json["list"])
#define getSubscriberCount(json) getDataFromJson(json)["total"].get<int>()
#define getSubscriberName(json) up.value().at("uname").get<string>()
#define forEachVideo(json,label) for(const auto& videoData : getDataFromJson(json)[label])
#define ofPerson "item"
#define ofSearch "result"

// Environments:
#define COOKIE "COOKIE"
#define USERAGENT "USERAGENT"