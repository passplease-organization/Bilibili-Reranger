#include "browse.h"

using namespace webAPI;

BrowseWorker NullWorker(nullptr);

const BrowseWorker &webAPI::nullWorker() {
    return NullWorker;
}

BrowseController BrowseController::controller("");

bool BrowseWorker::valid() const {
    return context != nullptr;
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

#define CRAWL_DATAMODE "data_mode"
#define CRAWL_TARGET "target"
Json CrawlAction::easyDescribe(const BrowseDataMode &mode, const std::string &description) {
    Json json;
    json[CRAWL_DATAMODE] = modeToString(mode);
    json[CRAWL_TARGET] = description;
    return json;
}

bool CrawlAction::validDescription(const BrowseDataMode &mode, const Json &json) {
    if (!json.is_object()
        || !json.contains(CRAWL_DATAMODE)
        || !json[CRAWL_DATAMODE].is_string()
        || json[CRAWL_DATAMODE].get<std::string>() != modeToString(mode)
        || !json.contains(CRAWL_TARGET))
        return false;

    const auto& description = json[CRAWL_TARGET];
    switch (mode) {
        case BrowseDataMode::DOM: {
            if (!description.is_string())
                return false;
            try {
                const auto selector = Json::parse(description.get<std::string>());
                if (!selector.is_object()
                    || !selector.contains("mode") || !selector["mode"].is_string()
                    || !selector.contains("param") || !selector["param"].is_string()
                    || !selector.contains("index")
                    || !(selector["index"].is_number_unsigned()
                        || (selector["index"].is_number_integer() && selector["index"].get<int>() >= 0)))
                    return false;
                const auto& selectMode = selector["mode"].get_ref<const std::string&>();
                return selectMode == "ID"
                    || selectMode == "CLASS"
                    || selectMode == "TAG"
                    || selectMode == "TAGNAME";
            } catch (...) {
                return false;
            }
        }
        case BrowseDataMode::HTTP_REQUEST:
        case BrowseDataMode::OTHER:
        case BrowseDataMode::NODATA:
            return description.is_string();
        default:
            return false;
    }
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

Json ScrollDownAction::_toJson() const {
    Json json;
    json[ScrollDownActionCount] = counts;
    return json;
}

Json WaitAction::_toJson() const {
    Json json;
    json["milliseconds"] = milliseconds;
    return json;
}

BrowseWorkingContext BrowseWorkingContext::EMPTY{};

std::string UrlAction::_name = "UrlAction";
std::string ClickAction::_name = "ClickAction";
std::string CrawlAction::_name = "CrawlAction";
std::string DoWhileAction::_name = "DoWhileAction";
std::string ScrollDownAction::_name = "ScrollDownAction";
std::string WaitAction::_name = "WaitAction";
