#include "bilibiliLogin.h"

#include <regex>
#include <cpr/api.h>
#include <cpr/body.h>
#include "../PortListener.h"

using namespace bilibili;

bilibiliLogin::bilibiliLogin(std::shared_ptr<const std::atomic<bool>> &stop)
: socialAPI(stop),
curl(webAPI::CurlHelper()) {
    curl.curlSetup();
}

string bilibiliLogin::login(const std::string& name, const std::string& password){
    if (name.empty() || password.empty()) {
        curl.setURL(BILIBILI_LOGIN_VERIFICATION);
        curl.connect();
        auto& json = curl.getJson();
        return json.contains("data") ? json["data"] : "错误返回Json";
    }else {
        string validate = INFO_BODY(BILIBILI_LOGIN_VERIFICATION_PARAMS_VALIDATE),
            seccode = INFO_BODY(BILIBILI_LOGIN_VERIFICATION_PARAMS_SECCODE),
            token = INFO_BODY(BILIBILI_LOGIN_VERIFICATION_PARAMS_TOKEN),
            challenge = INFO_BODY(BILIBILI_LOGIN_VERIFICATION_PARAMS_CHALLENGE);
        curl.setURL(BILIBILI_LOGIN_PUBLIC_KEY);
        curl.connect();
        if (stop) {
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
                    return "登录连接失败";
                }
                cookie = "";
                for (const auto& c : response.cookies)
                    cookie += c.GetValue();
                return validCOOKIE() ? "登录成功！" : "登录失败！";
            }
        }
        return "错误公钥信息！";
    }
}
