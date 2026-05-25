#include "webAPIs/browse.h"

#include <cpr/api.h>
#include <cpr/cprtypes.h>

#include "utils/Util.h"
#include "../Crawler.h"
#include "../../api/PortListener.h"

using namespace webAPI;

BrowseController::BrowseController(const std::string& ip) : browseIP(new cpr::Url(ip)){}

BrowseController::BrowseController(BrowseController &&other) noexcept
: browseIP(other.browseIP) {
    other.browseIP = nullptr;
}

BrowseController::BrowseController(const BrowseController &other)
: browseIP(new cpr::Url(*other.browseIP)){}

BrowseController& BrowseController::operator=(const BrowseController &other){
    if (&other != this) {
        this -> browseIP = new cpr::Url(*other.browseIP);
    }
    return *this;
}

BrowseController::~BrowseController() {
    delete browseIP;
}

inline cpr::Url append(const cpr::Url* const &url,const string& append) {
    return cpr::Url{url -> str() + append};
}

inline bool _testConnection(const cpr::Url* const &url) {
    if (!url)
        return false;
    auto&& response = cpr::Get(url -> operator+("/test"), cpr::Timeout{5000});
    return response.error.code == cpr::ErrorCode::OK;
}

bool BrowseController::testConnection() const {
    return _testConnection(browseIP);
}

Json BrowseController::perform(const BrowseWorker &worker,const unsigned int& workout) const {
    if (!_testConnection(browseIP))
        return {};
    Json json;
    json[CLIENT_ID] = crawlInfo -> clientId;
    json[PLATFORM] = crawlInfo -> client -> handler() -> support();
    json[CONTEXT] = worker.context -> toJson();
    json[BROWSER_TIMEOUT] = workout;
    auto&& workers = json[ActionsFlag];
    for (auto&& action : worker.actions) {
        workers.push_back(action -> toJson());
    }
    auto&& response = cpr::Post(
        *browseIP,
        cpr::Header{POST_JSON_HEADER},
        cpr::Body{
            json.dump()
        }
    );
    if (response.status_code != 200) {
        if (config<bool>(DETAILS)) {
            cppUtil::warn({false, nullptr}, "连接Browser故障，状态码：");
            cppUtil::warn(response.status_code);
            cppUtil::warn(response.reason);
        }
        return {};
    }
    try{
        if (auto&& back = Json::parse(response.text); SUCCESS_BROWSE_REQUEST(back))
            return back[ANSWER_DATA];
        else {
            if (back.contains(ANSWER_ERROR)) {
                auto&& error = back[ANSWER_ERROR];
                cppUtil::warn({false, nullptr}, "浏览器工作报错：\n名称：");
                cppUtil::warn(error["name"].get<string>());
                cppUtil::warn({false, nullptr}, "信息：");
                cppUtil::warn(error["message"].get<string>());
            }
            return {};
        }
    }catch (...) {
        return {};
    }
}

inline bool otherWork(const cpr::Url* const &browseIP,Nullable BrowseWorkingContext *const &context,const string& mode,Json json = Json()) {
    if (!_testConnection(browseIP))
        return false;
    json[CLIENT_ID] = crawlInfo -> clientId;
    json[PLATFORM] = crawlInfo -> client -> handler() -> support();
    if (context)
        json[CONTEXT] = context -> toJson();
    json[OTHER_MODE] = mode;
    auto&& response = cpr::Post(
        append(browseIP,"/other/" + mode),
        cpr::Header{POST_JSON_HEADER},
        cpr::Body{
            json.dump()
        }
    );
    if (response.status_code != 200)
        return false;
    auto&& back = Json::parse(response.text);
    return SUCCESS_BROWSE_REQUEST(back);
}

bool BrowseController::closeWorker(BrowseWorkingContext *const &context) const {
    return otherWork(browseIP,context,CLOSE_WORKER);
}

bool BrowseController::testContext(const BrowseWorker &worker) const {
    Json json;
    auto&& workers = json[ActionsFlag];
    for (auto&& action : worker.actions) {
        workers.push_back(action -> toJson());
    }
    return otherWork(browseIP,worker.context,TEST_CONTEXT,json);
}

cpr::Url BrowseController::openBridge(const std::string& clientID, cpr::Url url,const Json& screen) const {
    if (!_testConnection(browseIP))
        return {};
    Json json;
    json[CLIENT_ID] = clientID;
    json[PLATFORM] = crawlInfo -> client -> handler() -> support();
    #ifdef DEVELOP
        json[CONTEXT] = crawlInfo -> client -> handler() -> getContext() -> toJson();
    #else
        json[CONTEXT] = BrowseWorkingContext::EMPTY.toJson();
    #endif
    json[OTHER_MODE] = OPEN_BRIDGE;
    json[LOGIN_URL] = url.str();
    json[LOGIN_SCREEN_SIZE] = screen;
    auto&& response = cpr::Post(
        append(browseIP,"/" OPEN_BRIDGE),
        cpr::Header{POST_JSON_HEADER},
        cpr::Body{
            json.dump()
        }
    );
    if (response.status_code != 200) {
        if (config<bool>(DETAILS))
            cppUtil::warn("连接Browse登录失败");
        return {};
    }
    try {
        json = Json::parse(response.text);
        if (SUCCESS_BROWSE_REQUEST(json) && json.contains(OPEN_BRIDGE))
            return json[OPEN_BRIDGE].get<string>();
        if (config<bool>(DETAILS)) {
            cppUtil::warn({false, nullptr}, "错误Browser返回Json:");
            cppUtil::warn(json.dump());
        }
        return {};
    }catch (...) {
        return {};
    }
}
