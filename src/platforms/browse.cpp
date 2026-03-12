#include "webAPIs/browse.h"

#include <cpr/api.h>
#include <cpr/cprtypes.h>

#include "Util.h"
#include "../Crawler.h"
#include "../PortListener.h"

using namespace webAPI;

bool BrowseWorker::valid() const {
    return context != nullptr;
}

BrowseWorker NullWorker(nullptr);

const BrowseWorker &webAPI::nullWorker() {
    return NullWorker;
}

BrowseController BrowseController::controller("");

BrowseController::BrowseController(const std::string& ip) : browseIP(new cpr::Url(ip)){}

BrowseController::~BrowseController() {
    delete browseIP;
}

inline cpr::Url append(const cpr::Url* const &url,const string& append) {
    return cpr::Url{url -> str() + append};
}

inline bool _testConnection(const cpr::Url* const &url) {
    if (!url)
        return false;
    auto&& response = cpr::Get(append(url,"/test"), cpr::Timeout{5000});
    return response.error.code == cpr::ErrorCode::OK;
}

bool BrowseController::testConnection() const {
    return _testConnection(browseIP);
}

Json BrowseController::perform(const BrowseWorker &worker) const {
    if (!_testConnection(browseIP))
        return {};
    Json json;
    json[CLIENT_ID] = crawlInfo -> clientId;
    json[PLATFORM] = crawlInfo -> client -> handler() -> support();
    json[CONTEXT] = worker.context -> toJson();
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
    if (response.status_code != 200)
        return {};
    try{
        if (auto&& back = Json::parse(response.text); SUCCESS_BROWSE_REQUEST(back))
            return back[ANSWER_DATA];
        else {
            if (back.contains(ANSWER_ERROR)) {
                auto&& error = back[ANSWER_ERROR];
                warn("浏览器工作报错：\n名称：",false);
                warn(error["name"].get<string>().c_str());
                warn("信息：",false);
                warn(error["message"].get<string>().c_str());
            }
            return {};
        }
    }catch (...) {
        return {};
    }
}

inline bool otherWork(const cpr::Url* const &browseIP,BrowseWorkingContext *const &context,const string& mode) {
    if (!_testConnection(browseIP))
        return false;
    Json json;
    json[CLIENT_ID] = crawlInfo -> clientId;
    json[PLATFORM] = crawlInfo -> client -> handler() -> support();
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

bool BrowseController::testContext(BrowseWorkingContext *const &context) const {
    return otherWork(browseIP,context,TEST_CONTEXT);
}

cpr::Url BrowseController::openBridge(const std::string& clientID, cpr::Url url) const {
    if (!_testConnection(browseIP))
        return {};
    Json json;
    json[CLIENT_ID] = clientID;
    json[PLATFORM] = crawlInfo -> client -> handler() -> support();
    json[CONTEXT] = BrowseWorkingContext::EMPTY.toJson();
    json[OTHER_MODE] = OPEN_BRIDGE;
    json["platform_url"] = url.str();
    auto&& response = cpr::Post(
        append(browseIP,"/" OPEN_BRIDGE),
        cpr::Header{POST_JSON_HEADER},
        cpr::Body{
            json.dump()
        }
    );
    if (response.status_code != 200)
        return {};
    try {
        json = Json::parse(response.text);
        if (SUCCESS_BROWSE_REQUEST(json) && json.contains(OPEN_BRIDGE))
            return json[OPEN_BRIDGE].get<string>();
        return {};
    }catch (...) {
        return {};
    }
}

BrowseAction::~BrowseAction() = default;

Json BrowseAction::toJson() const {
    Json json;
    json[ACTION_FLAG] = name();
    json[ACTION_DATA] = _toJson();
    return json;
}

Json UrlAction::_toJson() const {
    Json json;
    json["url"] = url;
    return json;
}

Json ClickAction::_toJson() const {
    return selector.toJson();
}

Json CrawlAction::easyDescribe(const BrowseDataMode &mode, const std::string &description) {
    Json json;
    json[CRAWL_DATAMODE] = modeToString(mode);
    json[CRAWL_TARGET] = description;
    return json;
}

bool CrawlAction::validDescription(const BrowseDataMode &mode, const Json &json) {// TODO 完善
    return json.contains(CRAWL_DATAMODE) && json[CRAWL_DATAMODE].is_string() && json[CRAWL_DATAMODE].get<std::string>() == modeToString(mode)
        && json.contains(CRAWL_TARGET) && json[CRAWL_TARGET].is_string();
}

Json DoWhileAction::_toJson() const {
    Json json;
    json[DoWhileMode] = failOrSucceeded;
    json[DoWhileMAXWorkCount] = maxCount;
    auto&& workers = json[DoWhileAllActions];
    for (auto&& action : actions) {
        workers.push_back(action -> toJson());
    }
    return json;
}

BrowseWorkingContext BrowseWorkingContext::EMPTY{};

std::string UrlAction::_name = "UrlAction";
std::string ClickAction::_name = "ClickAction";
std::string CrawlAction::_name = "CrawlAction";
std::string DoWhileAction::_name = "DoWhileAction";
