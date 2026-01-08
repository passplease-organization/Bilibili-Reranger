#include "bilibiliLogin.h"
#include "bilibiliAPIs.h"
#include "config.h"
#include "loginAPI/crawler.h"
#include "../Crawler.h"

#if NEED_PORT

#include <iostream>
#include <regex>
#include <cpr/api.h>
#include <cpr/body.h>

#include "../PortListener.h"

using namespace webAPI;

bilibiliLogin::bilibiliLogin(std::shared_ptr<const std::atomic<bool>> &stop)
: socialAPI(stop),
curl(webAPI::CurlHelper()) {
    curl.curlSetup();
}

string bilibiliLogin::login(const std::string& name, const std::string& password,bool& failed){
#if TEST
    cookie = getenv(COOKIE);
    failed = false;
    return "登录成功";
#else
    if (name.empty() || password.empty()) {
        curl.setURL(BILIBILI_LOGIN_VERIFICATION);
        curl.connect();
        auto& json = curl.getJson();
        if (!json.contains("data")) {
            failed = true;
            return "错误返回Json";
        }
        failed = false;
        return json["data"];
    }else {
        string validate = INFO_BODY(BILIBILI_LOGIN_VERIFICATION_PARAMS_VALIDATE),
            seccode = INFO_BODY(BILIBILI_LOGIN_VERIFICATION_PARAMS_SECCODE),
            token = INFO_BODY(BILIBILI_LOGIN_VERIFICATION_PARAMS_TOKEN),
            challenge = INFO_BODY(BILIBILI_LOGIN_VERIFICATION_PARAMS_CHALLENGE);
        curl.setURL(BILIBILI_LOGIN_PUBLIC_KEY);
        curl.connect();
        if (stop) {
            failed = true;
            return "后台登录超时";
        }
        auto json = curl.getJson();
        if (json.contains("data") && json["data"].contains("hash") && json["data"].contains("key")) {
            auto&& salt = json["data"]["hash"].get<std::string>();
            auto&& key = json["data"]["key"].get<std::string>();
            std::regex keyRegex(R"(-----BEGIN PUBLIC KEY-----\n([A-Za-z0-9+/\n=]+)\n-----END PUBLIC KEY-----\n?)");
            std::smatch matches;
            if (std::regex_search(key, matches, keyRegex)) {
                key = "";
                for (auto& c : matches[1].str()) {
                    if (c != '\n')
                        key += c;
                }
                auto&& encryptPassword = webAPI::SimpleRSA::encrypt(key,salt + password);
                json.clear();
                json[URL_PARAMS_USERNAME] = name;
                json[URL_PARAMS_PASSWORD] = encryptPassword;
                json["keep"] = 0;
                json[BILIBILI_LOGIN_VERIFICATION_PARAMS_TOKEN] = token;
                json[BILIBILI_LOGIN_VERIFICATION_PARAMS_CHALLENGE] = challenge;
                json[BILIBILI_LOGIN_VERIFICATION_PARAMS_VALIDATE] = validate;
                json[BILIBILI_LOGIN_VERIFICATION_PARAMS_SECCODE] = seccode;
                auto&& response = cpr::Post(
                    cpr::Url{BILIBILI_LOGIN},
                    cpr::Body{json}
                );
                if (response.status_code != 200) {
                    failed = true;
                    return "登录连接失败";
                }
                cookie = "";
                for (const auto& c : response.cookies)
                    cookie += c.GetValue();
                if (validCOOKIE()) {
                    failed = false;
                    return "登录成功！";
                }
                failed = true;
                return "登录失败！";
            }
        }
        failed = true;
        return "错误公钥信息！";
    }
#endif
}

string bilibiliLogin::getURL(const crawlTask::Task* task) const {
    switch (task -> mode) {
        case crawlTask::WorkingMode::SUBSCRIBE: {
            string back = mySubscribers;
            back += "?vmid=";
            back += config<string>(VMID);
            return back;
        }
        case crawlTask::WorkingMode::TAG :
        case crawlTask::WorkingMode::SEARCH : {
            string back = searchVideos;
            back += "&page_size=";
            back += to_string(config<int>(SEARCH_PAGE_SIZE));
            back += "&keyword=";
            back += task -> keyword;
            return back;
        }
        default: return "";
    }
}

bool bilibiliLogin::dealJson(CrawlerHelper& helper, const Json& json, const crawlTask::Task* task) const {
    if (task == nullptr)
        return false;
    switch (task -> mode) {
        case crawlTask::WorkingMode::SUBSCRIBE : {
            if(!startWith(helper.nextURL().c_str(),videoByUser)) {

            #if MORE_DETAILS
                say("当前搜索的关注用户名：", false);
                say(task->keyword);
            #endif

                Json json_subs = helper.getSubscribers();
                for (auto &up: _getSubscribers(json_subs, false).items()) {

                #if MORE_DETAILS
                    say("当前比对up名：", false);
                    say(_getSubscriberName(up).c_str(), true, YELLOW);
                #endif

                    if (_getSubscriberName(up) == task->keyword) {
                        helper.clearNextURL();
                        string url(videoByUser);
                        url += "?vmid=";
                        url += to_string(up.value().at(VMID).get<int>());
                        url += "&ps=";
                        url += std::to_string(config<int>(SUBSCRIBE_SEARCH_VIDEO_COUNT));
                        helper.nextSearch(url);

                    #if MORE_DETAILS
                        say("关注用户爬取一次", true, BLUE);
                    #endif

                        helper.markMustCrawl();
                        return true;
                    }
                }
            }else{
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
    }
}

void bilibiliLogin::refreshSubscribers(CrawlerHelper& helper, const bool force) const {
    #if MORE_DETAILS
        say("开始准备关注博主名单");
    #endif

    if(helper.getSubscribers().empty() || force) {
        helper.clearNextURL();
        crawlTask::Task t("", 0, crawlTask::WorkingMode::SUBSCRIBE);
        helper.nextSearch(getURL(&t));
        do{
            helper.markMustCrawl();
            helper.connect(false);
            Json json = Json::parse(helper.rawData());

        #ifdef DEVELOP
            auto _json = to_string(json);
        #endif

            // 使用线程安全的方法添加订阅者
            auto newSubscribers = getDataFromJson(json).get<dataStore::Data>();
            helper.addSubscriber(newSubscribers);
            int count = _getSubscriberCount(json);
            int pages = count / 50 + 1;
            unsigned int nowPage = CrawlerHelper::parsePages(helper.nextURL());

        #if MORE_DETAILS
            say("关注博主刷新完成第",false);
            say(to_string(nowPage).c_str(),false);
            say("页");
        #endif

            if(nowPage >= pages)
                break;
            helper.advancePage(nowPage);
        }while(true);
        helper.clearData();
        helper.clearNextURL();
    }

    #if MORE_DETAILS
        say("关注博主名单已准备完成");
    #endif
}

#endif
