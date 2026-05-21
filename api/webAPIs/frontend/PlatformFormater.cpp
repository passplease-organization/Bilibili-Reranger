#include "PlatformFormater.h"

#include "../../interface.h"

namespace webAPI::formater {
    bool PlatformFormater::operator==(const PlatformFormater &other) const {
        return this -> name == other.name && this -> description == other.description;
    }

    void PlatformFormater::notSame(const PlatformFormater &other) const noexcept(false) {
        if (this -> name == other.name)
            throw SameException(SameException::Category::FORMATER_NAME);
    }

    void to_json(Json& json,const PlatformFormater& formater) {
        json["name"] = formater.name;
        json["description"] = formater.description;
        json[BODY_PARAMS_PLATFORM] = formater.platform;
    }

    FeedBack::FeedBack(const Json &json): FeedBack(json[BODY_PARAMS_VIDEO].get<webAPI::Video>(),json[BODY_PARAMS_SCORE].get<int>()) {
        if (json.contains("author")) {
            if (const auto& a = json["author"];a.contains("value") && a.contains(BODY_PARAMS_SCORE)) {
                author.author = a["value"].get<string>();
                author.score = clamp(a[BODY_PARAMS_SCORE].get<int>(),-MAX_SCORE,MAX_SCORE);
                author.has = true;
            }
        }
        if (json.contains("overall")) {
            if (const auto& o = json["overall"];o.contains("value") && o.contains(BODY_PARAMS_SCORE)) {
                overall.score = clamp(o["value"].get<int>(),-MAX_SCORE,MAX_SCORE);
                overall.once = o["once"].get<int>();
                overall.has = true;
            }
        }
        if (json.contains("tags")) {
            if (const auto& t = json["tags"]; t.is_array())
                tags = t.get<vector<string>>();
        }
    }
}
