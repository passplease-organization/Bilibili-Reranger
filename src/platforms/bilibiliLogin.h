#pragma once

#include "develop/flags.h"
#include "loginAPI/platforms.h"

#if NEED_PORT

#include "BilibiliInterface.h"
#include "loginAPI/socialAPI.h"

class CrawlerHelper;

namespace webAPI{
    #define BILIBILI_LOGIN_VERIFICATION "https://passport.bilibili.com/x/passport-login/captcha"
    #define BILIBILI_LOGIN_PUBLIC_KEY "https://passport.bilibili.com/x/passport-login/web/key"
    #define BILIBILI_LOGIN "https://passport.bilibili.com/x/passport-login/web/login"
    #define BILIBILI_LOGIN_VERIFICATION_PARAMS_VALIDATE "validate"
    #define BILIBILI_LOGIN_VERIFICATION_PARAMS_SECCODE "seccode"
    #define BILIBILI_LOGIN_VERIFICATION_PARAMS_TOKEN "token"
    #define BILIBILI_LOGIN_VERIFICATION_PARAMS_CHALLENGE "challenge"

    class bilibiliLogin : public webAPI::socialAPI {
        std::string cookie;

        webAPI::CurlHelper curl;

    public:
        bilibiliLogin(std::shared_ptr<const std::atomic<bool>>& stop);

        ~bilibiliLogin() override = default;

        string login(const std::string &name, const std::string &password,bool& failed) override;

        string getURL(const crawlTask::Task* task) const override;

        bool dealJson(CrawlerHelper& helper, const Json& json, const crawlTask::Task* task) const override;

        bool refreshSubscribers(CrawlerHelper& helper, bool force) const override;

        [[nodiscard]] const string& getCOOKIE() const override {
            return cookie;
        }

        [[nodiscard]] std::string support() const override {
            return BILIBILI;
        }
    };

    inline void registerBilibili() {
        webAPI::socialAPI::supportPlatform(BILIBILI,[](std::shared_ptr<const std::atomic<bool>>& stop) -> webAPI::socialAPI* {
            return new bilibiliLogin(stop);
        });
    }
}

#endif
