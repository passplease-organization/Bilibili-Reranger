#pragma once
#include <utility>
#include "../browse.h"
#include <functional>

#include "../../utils/BilibiliInterface.h"

namespace webAPI::formater {
    class SameException : public std::exception {
    public:
        enum struct Category {
            FORMATER_NAME
        };
    private:
        Category category;
    public:
        explicit SameException(const Category& c): category(c){}

        [[nodiscard]] const char *what() const noexcept override {
            switch (category) {
                case Category::FORMATER_NAME: return "重复的格式化名称";
                default: return "";
            }
        }
    };

    class PlatformFormater {
    public:
        std::string name;
        std::string description;
        std::string platform;
        std::function<BrowseWorker()> starter;
        std::function<BrowseWorker(const Json&)> judger;
        API PlatformFormater(std::string name,std::string description,std::function<BrowseWorker()> starter,
            std::function<BrowseWorker(const Json&)> judger = [](const Json&){return nullWorker();}
        ): name(std::move(name)),description(std::move(description)),starter(std::move(starter)),judger(std::move(judger)){}

        API bool operator==(const PlatformFormater &) const;
        API bool operator==(const string &n) const noexcept {
            return name == n;
        }

        API void notSame(const PlatformFormater&) const noexcept(false);
    };

    API void to_json(Json& json,const PlatformFormater& formater);

    extern "C" {
    struct FeedBack {
        webAPI::Video video;
        int score;

        #define MAX_SCORE 16
        FeedBack(webAPI::Video video,const int& score):video(std::move(video)),score(clamp(score,-MAX_SCORE,MAX_SCORE)) {}
        explicit FeedBack(const Json& json);

        ~FeedBack() = default;
    };
    }
}
