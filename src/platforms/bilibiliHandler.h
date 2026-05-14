#pragma once

#include "develop/flags.h"
#include "webAPIs/platforms.h"

#include "../../api/utils/BilibiliInterface.h"
#include "webAPIs/socialAPI.h"

class CrawlerHelper;

namespace webAPI{
    /**
    * These interfaces are deprecated now
    */
    #define BILIBILI_LOGIN_VERIFICATION "https://passport.bilibili.com/x/passport-login/captcha"
    #define BILIBILI_LOGIN_PUBLIC_KEY "https://passport.bilibili.com/x/passport-login/web/key"
    #define BILIBILI_LOGIN "https://passport.bilibili.com/x/passport-login/web/login"
    #define BILIBILI_LOGIN_VERIFICATION_CODE_CAPTCHA "https://passport.bilibili.com/x/safecenter/captcha/pre"
    #define BILIBILI_LOGIN_VERIFICATION_SEND "https://passport.bilibili.com/x/safecenter/common/sms/send"
    #define BILIBILI_LOGIN_VERIFY "https://passport.bilibili.com/x/safecenter/login/tel/verify"
    #define BILIBILI_LOGIN_EXCHANGE_COOKIE "https://passport.bilibili.com/x/passport-login/web/exchange_cookie"
    #define BILIBILI_LOGIN_COOKIE_GET_BVID3 "https://api.bilibili.com/x/frontend/finger/spi"
    #define BILIBILI_LOGIN_VERIFICATION_PARAMS_VALIDATE "validate"
    #define BILIBILI_LOGIN_VERIFICATION_PARAMS_SECCODE "seccode"
    #define BILIBILI_LOGIN_VERIFICATION_PARAMS_TOKEN "token"
    #define BILIBILI_LOGIN_VERIFICATION_PARAMS_CHALLENGE "challenge"
    #define BILIBILI_LOGIN_VERIFICATION_PARAMS_CODE "code"
    #define BILIBILI_LOGIN_FLAG_PHONE_VERIFICATION "phone_verification"
    /**
    * End here
    */

    #define BILIBILI_USER_MAIN_PAGE_URL "https://space.bilibili.com/"
    #define BILIBILI_USER_MAIN_PAGE(userId) (BILIBILI_USER_MAIN_PAGE_URL + (userId) + "/")
    #define BILIBILI_VIDEO_SEARCH_PAGE_URL "https://search.bilibili.com/" BILIBILI_VIDEO_ROUTER "/"
    #define BILIBILI_VIDEO_SEARCH_PAGE_HELPER(a,b,helper,...)  helper(a,b,__VA_ARGS__)
    #define BILIBILI_VIDEO_SEARCH_PAGE_HELPER1(keyword,page,...) (BILIBILI_VIDEO_SEARCH_PAGE_URL "?keyword=" + (keyword) + "&page=" + to_string(page))
    #define BILIBILI_VIDEO_SEARCH_PAGE_HELPER2(keyword,...) (BILIBILI_VIDEO_SEARCH_PAGE_URL "?keyword=" + (keyword))
    #define BILIBILI_VIDEO_SEARCH_PAGE(...) BILIBILI_VIDEO_SEARCH_PAGE_HELPER(__VA_ARGS__,BILIBILI_VIDEO_SEARCH_PAGE_HELPER1,BILIBILI_VIDEO_SEARCH_PAGE_HELPER2)

    #define SMS_TYPE "loginTelCheck"

    struct verificationCodeData {
        const string url;
        const string captcha_key;
        string tmp_code;
        string request_id;

        verificationCodeData(string url,string captcha_key);
    };

    class bilibiliHandler : public webAPI::socialAPI {
        webAPI::CurlHelper curl;

        verificationCodeData* data = nullptr;

    public:
        bilibiliHandler(std::shared_ptr<const std::atomic<bool>>& stop);

        ~bilibiliHandler() override = default;

        void setContextDomain() override;

        [[deprecated]] string login(const std::string &name, const std::string &password,bool& failed) override;
        [[nodiscard]] cpr::Url login(const string &clientID, bool &failed) override;

        BrowseWorker getWorker(const crawlTask::Task *task) const override;

        [[deprecated]] bool dealJson(CrawlerHelper& helper, const Json& json, const crawlTask::Task* task) const override;
        bool dealJson(const Json &json, const crawlTask::Task *task) const override;

        [[deprecated]] bool refreshSubscribers(CrawlerHelper& helper, bool force) const override;

        [[nodiscard]] const bool &prepare() override;

        [[nodiscard]] std::string support() const override {
            return BILIBILI;
        }

        void constexpr clearData() noexcept {
            if (data != nullptr) {
                delete data;
                data = nullptr;
            }
        }

        [[nodiscard]] bool validBrowse() const override;
    };

    inline void registerBilibili() {
        webAPI::socialAPI::supportPlatform(BILIBILI,[](std::shared_ptr<const std::atomic<bool>>& stop) -> webAPI::socialAPI* {
            const auto handler = new bilibiliHandler(stop);
            handler -> ensureContext();
            return handler;
        });
    }
}
