#include "bilibiliLogin.h"
#include "bilibiliAPIs.h"
#include "config.h"
#include "webAPIs/crawler.h"
#include "../Crawler.h"

#if NEED_PORT

#include <iostream>
#include <regex>
#include <boost/url/parse.hpp>
#include <cpr/api.h>
#include <cpr/payload.h>

#include "../PortListener.h"

using namespace webAPI;

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

bilibiliLogin::bilibiliLogin(std::shared_ptr<const std::atomic<bool>> &stop)
: socialAPI(stop),
curl(webAPI::CurlHelper()) {
    curl.curlSetup();
}

bool bilibiliLogin::setCOOKIE(const string& newCookie){
    cookie = newCookie;
    return validCOOKIE();
}

string bilibiliLogin::login(const std::string& name, const std::string& password,bool& failed){
#if EASY_LOGIN
    cookie = getenv(COOKIE);
    failed = false;
    return "登录成功";
#else
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
                    if (validCOOKIE()) {
                        failed = false;
                        if (config<bool>(DETAILS)) {
                            say("Cookie如下");
                            say(cookie.c_str());
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
                warn("服务器回复信息：");
                warn(response.text.c_str());
            }
        #ifdef DEVELOP
            say("登录报错，URL如下，可手动尝试验证：",true,BLUE);
            say(url.c_str(),true,BLUE);
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
        if (json.contains("data") && getDataFromJson(json).contains("hash") && getDataFromJson(json).contains("key")) {
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
                    {URL_PARAMS_USERNAME, name},
                    {URL_PARAMS_PASSWORD, encryptPassword},
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
            if (validCOOKIE()) {
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
                const string& url = getDataFromJson(j)["url"].get<string>();
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
                    warn("服务器回复信息：");
                    warn(response.text.c_str());
                }
            #ifdef DEVELOP
                say("登录报错，URL如下，可手动尝试验证：",true,BLUE);
                say(url.c_str(),true,BLUE);
            #endif
                return "请求验证码准备工作错误";
            }
            if (config<bool>(DETAILS)) {
                warn("回复body：");
                warn(response.text.c_str());
            }
            return "登录失败！";
        }
        failed = true;
        return "错误公钥信息！";
    }else {// TODO 改对应条件
        curl.setURL(BILIBILI_LOGIN_VERIFICATION);
        curl.connect();
        const auto& json = curl.getJson();
        if (!containsData(json)) {
            failed = true;
            if (config<bool>(DETAILS)) {
                say("获取验证码错误，得到：",false);
                say(json.dump().c_str());
            }
            return "错误返回Json";
        }
        failed = false;
        return getDataFromJson(json).dump();
    }
    failed = true;
    return "错误请求参数";
#endif
}

bool bilibiliLogin::validCOOKIE() const {
    return cookie.find("SESSDATA=") != string::npos &&
        cookie.find("bili_jct=") != string::npos;
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
    }
}

bool bilibiliLogin::refreshSubscribers(CrawlerHelper& helper, const bool force) const {
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
            Json json;
            try {
                json = Json::parse(helper.rawData());
            } catch (const std::exception& e) {
                warn("Parse subscribers response failed: ", false);
                warn(e.what());
                helper.clearData();
                helper.clearNextURL();
                return false;
            }

        #ifdef DEVELOP
            auto _json = to_string(json);
        #endif

            checkAndReturn(json);
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
    } else {
        return true;
    }

    #if MORE_DETAILS
        say("关注博主名单已准备完成");
    #endif
    return !helper.getSubscribers().empty() && helper.getSubscribers().valid();
}

#endif
