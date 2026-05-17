#include "bilibiliHandler.h"
#include "bilibiliAPIs.h"
#include "utils/config.h"
#include "webAPIs/crawler.h"
#include "../Crawler.h"

#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>
#include <boost/url/parse.hpp>
#include <cpr/api.h>
#include <cpr/payload.h>

#include "PortListener.h"
#include "webAPIs/browse.h"

using namespace webAPI;

namespace {
    string formatDurationFromSeconds(const long long duration) {
        const auto hours = duration / 3600;
        const auto minutes = duration % 3600 / 60;
        const auto seconds = duration % 60;

        std::stringstream stream;
        if (hours > 0) {
            stream << hours << ":"
                << std::setw(2) << std::setfill('0') << minutes << ":";
        }else {
            stream << minutes << ":";
        }
        stream << std::setw(2) << std::setfill('0') << seconds;
        return stream.str();
    }

    Json normalizeHomePageVideo(const Json& video) {
        Json normalized = video;

        if (video.contains("owner") && video["owner"].is_object()) {
            const auto& owner = video["owner"];
            if (!normalized.contains("author") && owner.contains("name"))
                normalized["author"] = owner["name"];
            if (!normalized.contains("mid") && owner.contains("mid")) {
                const auto mid = owner["mid"].get<long long>();
                normalized["mid"] = mid;
            }
        }

        if (!normalized.contains("description"))
            normalized["description"] = "";

        if (video.contains("stat") && video["stat"].is_object()) {
            const auto& stat = video["stat"];
            if (!normalized.contains("play") && stat.contains("view"))
                normalized["play"] = stat["view"];
            if (!normalized.contains("video_review") && stat.contains("danmaku"))
                normalized["video_review"] = stat["danmaku"];
        }

        if (!normalized.contains("length") && video.contains("duration"))
            normalized["length"] = formatDurationFromSeconds(video["duration"].get<long long>());

        if (!normalized.contains("arcurl") && video.contains("uri"))
            normalized["arcurl"] = video["uri"];

        return normalized;
    }

    bool isHomePageVideo(const Json& video) {
        return video.is_object()
            && video.contains("goto")
            && video["goto"].is_string()
            && video["goto"].get<string>() == "av"
            && video.contains("bvid")
            && video["bvid"].is_string()
            && !video["bvid"].get<string>().empty();
    }
}

verificationCodeData::verificationCodeData(string url,string captcha_key)
: url(std::move(url)),
captcha_key(std::move(captcha_key)) {
    auto parsed = boost::urls::parse_uri(this -> url);
    if (!parsed) {
        return;
    }
    auto params = parsed -> params();
    for (auto const& p : params) {
        if (p.key == "request_id") {
            request_id = std::string(p.value);
        } else if (p.key == "tmp_token") {
            tmp_code = std::string(p.value);
        }
    }
}

bilibiliHandler::bilibiliHandler(std::shared_ptr<const std::atomic<bool>> &stop)
: socialAPI(stop),
curl(webAPI::CurlHelper()) {
    curl.curlSetup();
}

void bilibiliHandler::setContextDomain() {
    context -> cookie_domain = ".bilibili.com";
    context -> cookie_path = "/";
}

string bilibiliHandler::login(const std::string& name, const std::string& password,bool& failed){
#if EASY_LOGIN
    context -> cookie = getenv(COOKIE);
    failed = false;
    return "登录成功";
#else
    string& cookie = context -> cookie; // avoid error
    if (BODY_CONTAIN(BILIBILI_LOGIN_VERIFICATION_PARAMS_CODE)) {
        failed = true;
        if (data == nullptr)
            return "数据未储存，请重新登录！";
        auto&& response = cpr::Post(
            cpr::Url{BILIBILI_LOGIN_VERIFY},
            cpr::Payload{
                {"tmp_code",data -> tmp_code},
                {"captcha_key",data -> captcha_key},
                {"type",SMS_TYPE},
                {"code",INFO_BODY(BILIBILI_LOGIN_VERIFICATION_PARAMS_CODE)},
                {"request_id",data -> request_id},
                {"source","risk"}
            }
        );
        if (!stop -> load() && response.status_code == 200) {
            auto&& j = Json::parse(response.text);
            if (containsData(j)) {
                response = cpr::Post(
                    cpr::Url{BILIBILI_LOGIN_EXCHANGE_COOKIE},
                    cpr::Payload{
                        {"source",{"risk"}},
                        {"code",getDataFromJson(j)["code"].get<string>()}
                    }
                );
                if (response.status_code == 200) {
                    cookie = "";
                    for (const auto& c : response.cookies) {
                        const auto& name = c.GetName();
                        const auto& value = c.GetValue();
                        if (!name.empty()) {
                            cookie += name;
                            cookie += "=";
                            cookie += value;
                            cookie += "; ";
                        }
                    }
                    curl.setURL(BILIBILI_LOGIN_COOKIE_GET_BVID3);
                    curl.connect();
                    if (const auto& json = curl.getJson(); containsData(json)) {
                        cookie += "buvid3=";
                        cookie += getDataFromJson(json)["b_3"].get<string>();
                        cookie += " ;buvid4=";
                        cookie += getDataFromJson(json)["b_4"].get<string>();
                        cookie += "; ";
                    }
                    if (validBrowse()) {
                        failed = false;
                        if (config<bool>(DETAILS)) {
                            cppUtil::say("Cookie如下");
                            cppUtil::say(cookie);
                        }
                        return "登录成功！";
                    }
                }
                return "交换COOKIE错误";
            }
            return "错误B站回复";
        }
        return "后端访问错误";
    }else if (
        BODY_CONTAIN(BILIBILI_LOGIN_VERIFICATION_PARAMS_VALIDATE) &&
        BODY_CONTAIN(BILIBILI_LOGIN_VERIFICATION_PARAMS_SECCODE) &&
        BODY_CONTAIN(BILIBILI_LOGIN_VERIFICATION_PARAMS_TOKEN) &&
        BODY_CONTAIN(BILIBILI_LOGIN_VERIFICATION_PARAMS_CHALLENGE)
    ){
        string validate = INFO_BODY(BILIBILI_LOGIN_VERIFICATION_PARAMS_VALIDATE),
            seccode = INFO_BODY(BILIBILI_LOGIN_VERIFICATION_PARAMS_SECCODE),
            token = INFO_BODY(BILIBILI_LOGIN_VERIFICATION_PARAMS_TOKEN),
            challenge = INFO_BODY(BILIBILI_LOGIN_VERIFICATION_PARAMS_CHALLENGE);
        if (BODY_CONTAIN(BILIBILI_LOGIN_FLAG_PHONE_VERIFICATION)) {
            failed = true;
            if (data == nullptr)
                return "数据未储存，请重新登录！";
            string url = data -> url;
            auto&& response = cpr::Post(
                cpr::Url{BILIBILI_LOGIN_VERIFICATION_SEND},
                cpr::Payload{
                    {"tmp_code",data -> tmp_code},
                    {"sms_type",SMS_TYPE},
                    {"recaptcha_token",token},
                    {"gee_challenge",challenge},
                    {"gee_validate",validate},
                    {"gee_seccode",seccode}
                }
            );
            if (!stop -> load() && response.status_code == 200) {
                auto&& j = Json::parse(response.text);
                if (containsData(j)) {
                    failed = false;
                    clearData();
                    data = new verificationCodeData(url,getDataFromJson(j)["captcha_key"].get<string>());
                    return "{\"status\":\"手机验证，等待验证码\"}";
                }
            }
            if (config<bool>(DETAILS)) {
                cppUtil::warn("服务器回复信息：");
                cppUtil::warn(response.text);
            }
        #ifdef DEVELOP
            cppUtil::say({true, BLUE}, "登录报错，URL如下，可手动尝试验证：");
            cppUtil::say({true, BLUE}, url);
        #endif
            return "发送验证码错误";
        }
        curl.setURL(BILIBILI_LOGIN_PUBLIC_KEY);
        curl.connect();
        if (stop -> load()) {
            failed = true;
            return "后台登录超时";
        }
        const auto& json = curl.getJson();
        if (containsData(json) && getDataFromJson(json).contains("hash") && getDataFromJson(json).contains("key")) {
            auto&& salt = getDataFromJson(json)["hash"].get<std::string>();
            auto&& key = getDataFromJson(json)["key"].get<std::string>();
            auto&& encryptPassword = SimpleRSA::encrypt(key,salt + password);
            cpr::Header headers{{"Content-Type", "application/x-www-form-urlencoded"}};
            if (!user_agent.empty()) {
                headers["User-Agent"] = user_agent;
            }
            auto&& response = cpr::Post(
                cpr::Url{BILIBILI_LOGIN},
                cpr::Payload{
                    // {URL_PARAMS_USERNAME, name},
                    // {URL_PARAMS_PASSWORD, encryptPassword},
                    {"keep", "0"},
                    {BILIBILI_LOGIN_VERIFICATION_PARAMS_TOKEN, token},
                    {BILIBILI_LOGIN_VERIFICATION_PARAMS_CHALLENGE, challenge},
                    {BILIBILI_LOGIN_VERIFICATION_PARAMS_VALIDATE, validate},
                    {BILIBILI_LOGIN_VERIFICATION_PARAMS_SECCODE, seccode}
                },
                headers
            );
            if (response.status_code != 200) {
                failed = true;
                return "登录连接失败";
            }
            cookie = "";
            for (const auto& c : response.cookies) {
                const auto& _name = c.GetName();
                const auto& value = c.GetValue();
                if (!_name.empty()) {
                    cookie += _name;
                    cookie += "=";
                    cookie += value;
                    cookie += "; ";
                }
            }
            if (validBrowse()) {
                failed = false;
                return "登录成功！";
            }
            if (stop -> load()) {
                failed = true;
                return "登录超时";
            }
            failed = true;
            auto&& j = Json::parse(response.text);
            if (containsData(j) && getDataFromJson(j).contains("url")) {
                const std::string& url = getDataFromJson(j)["url"].get<std::string>();
                response = cpr::Post(
                    cpr::Url{BILIBILI_LOGIN_VERIFICATION_CODE_CAPTCHA},
                    headers
                );
                if (response.status_code == 200) {
                    j = Json::parse(response.text);
                    if (containsData(j)) {
                        clearData();
                        data = new verificationCodeData(url,"");
                        failed = false;
                        return j.dump();
                    }
                }
                if (config<bool>(DETAILS)) {
                    cppUtil::warn("服务器回复信息：");
                    cppUtil::warn(response.text);
                }
            #ifdef DEVELOP
                cppUtil::say({true, BLUE}, "登录报错，URL如下，可手动尝试验证：");
                cppUtil::say({true, BLUE}, url);
            #endif
                return "请求验证码准备工作错误";
            }
            if (config<bool>(DETAILS)) {
                cppUtil::warn("回复body：");
                cppUtil::warn(response.text);
            }
            return "登录失败！";
        }
        failed = true;
        return "错误公钥信息！";
    }else {
        curl.setURL(BILIBILI_LOGIN_VERIFICATION);
        curl.connect();
        const auto& json = curl.getJson();
        if (!containsData(json)) {
            failed = true;
            if (config<bool>(DETAILS)) {
                cppUtil::say({false, nullptr}, "获取验证码错误，得到：");
                cppUtil::say(json.dump());
            }
            return "错误返回Json";
        }
        failed = false;
        return getDataFromJson(json).dump();
    }
#endif
    failed = true;
    return "错误请求参数";
}

cpr::Url bilibiliHandler::login(const string &clientID, bool &failed) {
    return getController().openBridge(clientID,cpr::Url(BILIBILI_USER_MAIN_PAGE_URL),INFO_BODY(LOGIN_SCREEN_SIZE));
}

BrowseWorker bilibiliHandler::getWorker(const crawlTask::Task *task) const {
    // switch (task -> mode) {
    //     case crawlTask::WorkingMode::SUBSCRIBE: {
    //         string back = deprecated_mySubscribers;
    //         back += "?vmid=";
    //         back += config<string>(VMID);
    //         return back;
    //     }
    //     case crawlTask::WorkingMode::TAG :
    //     case crawlTask::WorkingMode::SEARCH : {
    //         string back = deprecated_searchVideos;
    //         back += "&page_size=";
    //         back += to_string(config<int>(SEARCH_PAGE_SIZE));
    //         back += "&keyword=";
    //         back += task -> keyword;
    //         return back;
    //     }
    //     default: return "";
    // }
    if (task == nullptr)
        return nullWorker();
    switch (task -> mode) {
        case crawlTask::WorkingMode::SUBSCRIBE: {
            int id = 0;
            if (_subscribers.contains(task -> keyword))
                id = _subscribers[task -> keyword].get<int>();
            else {
                cppUtil::warn({false, nullptr}, "没有这个博主：");
                cppUtil::warn(task -> keyword);
                return nullWorker();
            }
            return {
                context,
                UrlAction{BILIBILI_USER_MAIN_PAGE(to_string(id))},
                ClickAction{ElementSelector::SelectMode::CLASS,"nav-tab__item",2},
                DoWhileAction{false,5,
                    CrawlAction{BrowseAction::BrowseDataMode::HTTP_REQUEST,"api.bilibili.com/x/space/wbi/arc/search"},
                    ClickAction{ElementSelector::SelectMode::CLASS,"vui_button vui_pagenation--btn vui_pagenation--btn-side",1}
                },
                CrawlAction{BrowseAction::BrowseDataMode::HTTP_REQUEST,"api.bilibili.com/x/space/wbi/arc/search"},
            };
        }
        case crawlTask::WorkingMode::CATEGORY: {
            return {
                context,
                UrlAction{BILIBILI_MAIN_PAGE_URL "c/" + string(task -> keyword)},
                ScrollDownAction{},
                DoWhileAction{false,5,
                    CrawlAction{BrowseAction::BrowseDataMode::HTTP_REQUEST,"api.bilibili.com/x/web-interface/region"},
                    ScrollDownAction{}
                },
                CrawlAction{BrowseAction::BrowseDataMode::HTTP_REQUEST,"api.bilibili.com/x/web-interface/region"},
            };
        }
        case crawlTask::WorkingMode::TAG:
        case crawlTask::WorkingMode::SEARCH: {
            return {
                context,
                UrlAction{BILIBILI_VIDEO_SEARCH_PAGE(string(task -> keyword),task -> workCount() * 5 + 1)},
                ClickAction{ElementSelector::SelectMode::CLASS,"vui_button vui_pagenation--btn vui_pagenation--btn-side",1},
                DoWhileAction{false,5,
                    CrawlAction{BrowseAction::BrowseDataMode::HTTP_REQUEST,"api.bilibili.com/x/web-interface/wbi"},
                    ClickAction{ElementSelector::SelectMode::CLASS,"vui_button vui_pagenation--btn vui_pagenation--btn-side",1}
                },
                CrawlAction{BrowseAction::BrowseDataMode::HTTP_REQUEST,"api.bilibili.com/x/web-interface/wbi"},
            };
        }
        case crawlTask::WorkingMode::HOME_PAGE_FILTER: {
            return {
                context,
                UrlAction{BILIBILI_MAIN_PAGE_URL},
                ClickAction{ElementSelector::SelectMode::CLASS,"primary-btn roll-btn"},
                DoWhileAction{false,5,
                    CrawlAction{BrowseAction::BrowseDataMode::HTTP_REQUEST,"api.bilibili.com/x/web-interface/wbi"},
                    ClickAction{ElementSelector::SelectMode::CLASS,"primary-btn roll-btn"}
                },
                CrawlAction{BrowseAction::BrowseDataMode::HTTP_REQUEST,"api.bilibili.com/x/web-interface/wbi"},
            };
        }
        default: return nullWorker();
    }
}

bool bilibiliHandler::dealJson(CrawlerHelper& helper, const Json& json, const crawlTask::Task* task) const {
    return false;
    /*if (task == nullptr)
        return false;
    switch (task -> mode) {
        case crawlTask::WorkingMode::SUBSCRIBE : {
            if(!startWith(helper.nextURL().c_str(),deprecated_videoByUser)) {

            #if MORE_DETAILS
                cppUtil::say({false, nullptr}, "当前搜索的关注用户名：");
                cppUtil::say(task->keyword);
            #endif

                Json json_subs = helper.getSubscribers();
                for (auto &up: _getSubscribers(json_subs, false).items()) {

                #if MORE_DETAILS
                    cppUtil::say({false, nullptr}, "当前比对up名：");
                    cppUtil::say({true, YELLOW}, _getSubscriberName(up));
                #endif

                    if (_getSubscriberName(up) == task->keyword) {
                        helper.clearNextURL();
                        string url(deprecated_videoByUser);
                        url += "?vmid=";
                        url += to_string(up.value().at(VMID).get<int>());
                        url += "&ps=";
                        url += std::to_string(config<int>(SUBSCRIBE_SEARCH_VIDEO_COUNT));
                        helper.nextSearch(url);

                    #if MORE_DETAILS
                        cppUtil::say({true, BLUE}, "关注用户爬取一次");
                    #endif

                        helper.markMustCrawl();
                        return true;
                    }
                }
            }else{
                checkAndReturn(json);
                checkResult(json);
                const auto* group = crawlTask::getGroup();
                const char* groupName = group == nullptr ? "" : group -> name;
                const char* groupPlatform = group == nullptr ? "" : group -> platform;
                forEachVideo(json,ofPerson){
                    const auto& video = webAPI::Video::fromJson(videoData);
                    webAPI::setVideo(&video);
                    if(roughCheckVideo() && finalCheckVideo())
                        webAPI::keepVideo(video,groupName,groupPlatform);
                    webAPI::clearVideo();
                }
                helper.clearNextURL();
                return crawlTask::nextTask(true) != nullptr;
            }
            return false;
        }
        case crawlTask::WorkingMode::TAG : {
            checkAndReturn(json);
            checkResult(json);
            const auto* group = crawlTask::getGroup();
            const char* groupName = group == nullptr ? "" : group -> name;
            const char* groupPlatform = group == nullptr ? "" : group -> platform;
            forEachVideo(json,ofSearch){
                const auto& video = webAPI::Video::fromJson(videoData);
                if(videoData["tag"].get<string>().find(task -> keyword) == string::npos)
                    continue;
                webAPI::setVideo(&video);
                if(roughCheckVideo() && finalCheckVideo())
                    webAPI::keepVideo(video,groupName,groupPlatform);
                webAPI::clearVideo();
            }
            if(webAPI::enoughVideo(groupName,groupPlatform)) {
                helper.clearNextURL();
                return crawlTask::nextTask(true) != nullptr;
            }
            std::cout << getDataFromJson(json)["next"] << std::endl;
            int page = getDataFromJson(json)["next"].get<int>();
            helper.advancePage(page);
            return true;
        }case crawlTask::WorkingMode::SEARCH : {
            checkAndReturn(json);
            checkResult(json);
            const auto* group = crawlTask::getGroup();
            const char* groupName = group == nullptr ? "" : group -> name;
            const char* groupPlatform = group == nullptr ? "" : group -> platform;
            forEachVideo(json,ofSearch){
                const auto& video = webAPI::Video::fromJson(videoData);
                webAPI::setVideo(&video);
                if(roughCheckVideo() && finalCheckVideo())
                    webAPI::keepVideo(video,groupName,groupPlatform);
                webAPI::clearVideo();
            }
            if(webAPI::enoughVideo(groupName,groupPlatform)) {
                helper.clearNextURL();
                return crawlTask::nextTask(true) != nullptr;
            }
            int page = getDataFromJson(json)["next"].get<int>();
            helper.advancePage(page);
            return true;
        }
        default: return false;
    }*/
}

bool bilibiliHandler::dealJson(const Json &json, const crawlTask::Task *task) const {
    if (!validWorkerData(json))
        return false;
    const auto* group = crawlTask::getGroup();
    const char* groupName = group == nullptr ? "" : group -> name;
    switch (task -> mode) {
        case crawlTask::WorkingMode::SUBSCRIBE : {
            const auto& crawlData = json[2];
            if (!validWhileData(crawlData))
                return false;
            bool empty = true;
            auto resolver = [this,&empty,&groupName](const Json& data) -> void {
                empty = false;
                #define SEARCH_BILIBILI_VIDEOS_SUB "vlist"
                if (!containsData(data) || !containsList(data))
                    return;
                const auto& list = _getListFromData(data,true);
                if (!list.contains(SEARCH_BILIBILI_VIDEOS_SUB))
                    return;
                if (const auto& videos =  list[SEARCH_BILIBILI_VIDEOS_SUB]; videos.is_array())
                    for (const auto& video : videos)
                        if(const auto& v = Video::fromJson(video); checkVideo(v))
                            webAPI::keepVideo(v,groupName,this -> support().c_str());
            };
            forEachWhileData(crawlData) {
                auto stringData = crawlData.dump();
                const auto& _data = _crawlData[0];
                if (_data.is_object()) {
                    if (!EmptyCrawlData(_data))
                        resolver(_data[CrawlData]);
                }else if (_data.is_array())
                    for (const auto& __data : _data)
                        if (!EmptyCrawlData(__data))
                            resolver(__data[CrawlData]);
            }
            const auto& j = json[3];
            if (j.is_object())
                if (!EmptyCrawlData(j))
                    resolver(j[CrawlData]);
            if (empty)
                cppUtil::warn("浏览器爬取的数据都是空的！",crawlData);
            return !empty;
        }
        case crawlTask::WorkingMode::CATEGORY : {
            const auto& data = json[2];
            if (!validWhileData(data))
                return false;
            bool empty = true;
            auto resolver = [this,&empty,&groupName](const Json& data) -> void {
                empty = false;
                #define SEARCH_BILIBILI_VIDEOS_CATEGORY "archives"
                if (!containsData(data) || !getDataFromJson(data).contains(SEARCH_BILIBILI_VIDEOS_CATEGORY))
                    return;
                const auto& videos = getDataFromJson(data)[SEARCH_BILIBILI_VIDEOS_CATEGORY];
                if (!videos.is_array())
                    return;
                for (const auto& video : videos)
                    if(const auto& v = Video::fromJson(video); checkVideo(v))
                        webAPI::keepVideo(v,groupName,this -> support().c_str());
            };
            forEachWhileData(data) {
                auto stringData = data.dump();
                const auto& _data = _crawlData[0];
                if (_data.is_object()) {
                    if (!EmptyCrawlData(_data))
                        resolver(_data[CrawlData]);
                }else if (_data.is_array())
                    for (const auto& __data : _data)
                        if (!EmptyCrawlData(__data))
                            resolver(__data[CrawlData]);
            }
            const auto& j = json[3];
            if (j.is_object())
                if (!EmptyCrawlData(j))
                    resolver(j[CrawlData]);
            if (empty)
                cppUtil::warn("浏览器爬取的数据都是空的！",data);
            return !empty;
        }
        case crawlTask::WorkingMode::TAG :
        case crawlTask::WorkingMode::SEARCH : {
            const auto& data = json[2];
            if (!validWhileData(data))
                return false;
            bool empty = true;
            auto resolver = [this,&empty,&groupName](const Json& data) -> void {
                empty = false;
                #define SEARCH_BILIBILI_VIDEOS_SEARCH "result"
                if (!containsData(data) || !getDataFromJson(data).contains(SEARCH_BILIBILI_VIDEOS_SEARCH))
                    return;
                const auto& videos = getDataFromJson(data)[SEARCH_BILIBILI_VIDEOS_SEARCH];
                if (!videos.is_array())
                    return;
                for (const auto& video : videos) {
                    if(const auto& v = Video::fromJson(video); checkVideo(v))
                        webAPI::keepVideo(v,groupName,this -> support().c_str());
                }
            };
            forEachWhileData(data) {
                auto stringData = data.dump();
                const auto& _data = _crawlData[0];
                if (_data.is_object()) {
                    if (!EmptyCrawlData(_data))
                        resolver(_data[CrawlData]);
                }else if (_data.is_array())
                    for (const auto& __data : _data)
                        if (!EmptyCrawlData(__data))
                            resolver(__data[CrawlData]);
            }
            const auto& j = json[3];
            if (j.is_object())
                if (!EmptyCrawlData(j))
                    resolver(j[CrawlData]);
            if (empty)
                cppUtil::warn("浏览器爬取的数据都是空的！",data);
            return !empty;
        }
        case crawlTask::WorkingMode::HOME_PAGE_FILTER : {
            const auto& data = json[2];
            if (!validWhileData(data))
                return false;
            bool empty = true;
            auto resolver = [this,&empty,&groupName](const Json& data) -> void {
                empty = false;
                #define HOME_BILIBILI_VIDEOS_SEARCH "item"
                if (!containsData(data) || !getDataFromJson(data).contains(HOME_BILIBILI_VIDEOS_SEARCH))
                    return;
                const auto& videos = getDataFromJson(data)[HOME_BILIBILI_VIDEOS_SEARCH];
                if (!videos.is_array())
                    return;
                for (const auto& video : videos) {
                    if (!isHomePageVideo(video))
                        continue;
                    if(const auto& v = Video::fromJson(video); checkVideo(v))
                        webAPI::keepVideo(v,groupName,this -> support().c_str());
                }
            };
            forEachWhileData(data) {
                auto stringData = data.dump();
                const auto& _data = _crawlData[0];
                if (_data.is_object()) {
                    if (!EmptyCrawlData(_data))
                        resolver(_data[CrawlData]);
                }else if (_data.is_array())
                    for (const auto& __data : _data)
                        if (!EmptyCrawlData(__data))
                            resolver(__data[CrawlData]);
            }
            const auto& j = json[3];
            if (j.is_object())
               if (!EmptyCrawlData(j))
                   resolver(j[CrawlData]);
            if (empty)
                cppUtil::warn("浏览器爬取的数据都是空的！",data);
            return !empty;
        }
        default: return false;
    }
}

bool bilibiliHandler::refreshSubscribers(CrawlerHelper& helper, const bool force) const {
    #if MORE_DETAILS
        cppUtil::say("开始准备关注博主名单");
    #endif

    if(helper.getSubscribers().empty() || force) {
        helper.clearNextURL();
        crawlTask::Task t("", 0, crawlTask::WorkingMode::SUBSCRIBE);
        // helper.nextSearch(getWorker(&t).context -> url);
        do{
            helper.markMustCrawl();
            helper.connect(false);
            Json json;
            try {
                json = Json::parse(helper.rawData());
            } catch (const std::exception& e) {
                cppUtil::warn({false, nullptr}, "Parse subscribers response failed: ");
                cppUtil::warn(e.what());
                helper.clearData();
                helper.clearNextURL();
                return false;
            }

            checkAndReturn(json);
            auto newSubscribers = getDataFromJson(json);
            helper.addSubscriber(newSubscribers);
            int count = _getSubscriberCount(json);
            int pages = count / 50 + 1;
            unsigned int nowPage = CrawlerHelper::parsePages(helper.nextURL());

        #if MORE_DETAILS
            cppUtil::say({false, nullptr}, "关注博主刷新完成第");
            cppUtil::say({false, nullptr}, nowPage);
            cppUtil::say("页");
        #endif

            if(nowPage >= pages)
                break;
            helper.advancePage(nowPage);
        }while(true);
        helper.clearData();
        helper.clearNextURL();
    } else {
        return true;
    }

    #if MORE_DETAILS
        cppUtil::say("关注博主名单已准备完成");
    #endif
    return !helper.getSubscribers().empty();
}

const bool &bilibiliHandler::prepare() {
    if (prepared)
        return prepared;
    const auto& _crawlData = webAPI::getController().perform({
        context,
        UrlAction{BILIBILI_USER_MAIN_PAGE_URL},
        ClickAction{ElementSelector::SelectMode::CLASS,"nav-statistics__item jumpable",0},
        DoWhileAction{false,100,
            // For some reason the CrawlAction doesn't do well, always be one step behind
            CrawlAction{BrowseAction::BrowseDataMode::HTTP_REQUEST,"api.bilibili.com/x/relation/followings"},
            ClickAction{ElementSelector::SelectMode::CLASS,"vui_button vui_pagenation--btn vui_pagenation--btn-side",1}
        },
        // Last CrawlAction to catch the last page of subscribers
        CrawlAction{BrowseAction::BrowseDataMode::HTTP_REQUEST,"api.bilibili.com/x/relation/followings"}
    });
    if (!validWorkerData(_crawlData)) {
        #ifdef MORE_DETAILS
            cppUtil::warn("Wrong browser answer:");
            cppUtil::warn(_crawlData.dump());
        #endif
        prepared = false;
        return prepared;
    }
    const auto& crawlData = _crawlData[2];
    if (!validWhileData(crawlData)) {
        #ifdef MORE_DETAILS
            cppUtil::warn("Wrong browser answer:");
            cppUtil::warn(_crawlData.dump());
        #endif
        prepared = false;
        return prepared;
    }
    _subscribers.clear();
    auto dataResolver = [this](const Json& _data) -> void {
        if (EmptyCrawlData(_data))
            return;
        const auto& data = _data[CrawlData];
        if (!containsData(data) || !containsList(data))
            return;
        for (const auto& d : _getListFromData(data,true)) {
            if (_hasSubscriberMid(d) && _hasSubscriberName(d))
                _subscribers[_getSubscriberName(d)] = _getSubscriberMid(d);
        }
    };
    forEachWhileData(crawlData,__data)
        dataResolver(__data[0]);
    dataResolver(_crawlData[3]);
    prepared = !_subscribers.empty();
    return prepared;
}

bool bilibiliHandler::validBrowse() const {
    return getController().testContext({
        context,
        UrlAction{BILIBILI_USER_MAIN_PAGE_URL},
        ClickAction{ElementSelector::SelectMode::CLASS,"nav-statistics__item jumpable",0}
    });
}
