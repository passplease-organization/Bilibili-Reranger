#pragma once

#include <utility>

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
    #define BILIBILI_LOGIN_VERIFICATION_CODE_CAPTCHA "https://passport.bilibili.com/x/safecenter/captcha/pre"
    #define BILIBILI_LOGIN_VERIFICATION_SEND "https://passport.bilibili.com/x/safecenter/common/sms/send"
    #define BILIBILI_LOGIN_VERIFY "https://passport.bilibili.com/x/safecenter/login/tel/verify"
    #define BILIBILI_LOGIN_EXCHANGE_COOKIE "https://passport.bilibili.com/x/passport-login/web/exchange_cookie"
    #define BILIBILI_LOGIN_VERIFICATION_PARAMS_VALIDATE "validate"
    #define BILIBILI_LOGIN_VERIFICATION_PARAMS_SECCODE "seccode"
    #define BILIBILI_LOGIN_VERIFICATION_PARAMS_TOKEN "token"
    #define BILIBILI_LOGIN_VERIFICATION_PARAMS_CHALLENGE "challenge"
    #define BILIBILI_LOGIN_VERIFICATION_PARAMS_CODE "code"
    #define BILIBILI_LOGIN_FLAG_PHONE_VERIFICATION "phone_verification"

    #define SMS_TYPE "loginTelCheck"

    struct verificationCodeData {
        const string url;
        const string captcha_key;
        string tmp_code;
        string request_id;

        verificationCodeData(string url,string captcha_key);
    };

    class bilibiliLogin : public webAPI::socialAPI {
        std::string cookie;

        webAPI::CurlHelper curl;

        verificationCodeData* data = nullptr;

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

        [[nodiscard]] bool validCOOKIE() const override;

        [[nodiscard]] std::string support() const override {
            return BILIBILI;
        }

        void constexpr clearData() noexcept {
            if (data != nullptr) {
                delete data;
                data = nullptr;
            }
        }
    };

    inline void registerBilibili() {
        webAPI::socialAPI::supportPlatform(BILIBILI,[](std::shared_ptr<const std::atomic<bool>>& stop) -> webAPI::socialAPI* {
            return new bilibiliLogin(stop);
        });
    }
}

#endif
