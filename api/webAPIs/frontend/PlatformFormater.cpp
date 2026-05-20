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

    FeedBack::FeedBack(const Json &json): FeedBack(json[BODY_PARAMS_NAME].get<string>().c_str(),webAPI::Video::fromJson(json[BODY_PARAMS_VIDEO]),json[BODY_PARAMS_SCORE].get<int>()) {}
}
