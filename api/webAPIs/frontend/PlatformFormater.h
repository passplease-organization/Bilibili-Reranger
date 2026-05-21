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
        /**
         * @memberof video 反馈的主要视频
         * @memberof score 笼统评分，负为讨厌，最大16，最小-16
         * @memberof author 关于作者的字段，可能有，可能无
         * @memberof overall 总览，基本情况，一般部分的情况总结
         * @memberof tags 可能传递了的客户端发来的需要处理和关注的视频标签
         */
        webAPI::Video video;
        int score;

        struct Author {
            /**
             * @memberof author 视频博主名称
             * @memberof score 此项打分，最大16，最小-16
             * @memberof has 有无传递此项，false表示没传递，不用管
             */
            string author;
            int score;
            bool has;

            inline const char* getAuthor() const {
                return author.c_str();
            }
        };
        Author author{"",0,false};
        struct Overall {
            /**
             * @memberof score 此项重要程度专门打分
             * @memberof once 是否只是针对这一个视频
             * @memberof has 有没有这一项
             */
            int score;
            bool once;
            bool has;
        };
        Overall overall{0,false,false};

        vector<string> tags;

        #define MAX_SCORE 16
        FeedBack(webAPI::Video video,const int& score):video(std::move(video)),score(clamp(score,-MAX_SCORE,MAX_SCORE)) {}
        explicit FeedBack(const Json& json);

        ~FeedBack() = default;

        [[nodiscard]] const char* getTags(const int& index) const {
            if (index >= 0 && index < tags.size())
                return tags[index].c_str();
            return nullptr;
        }
    };
    }
}
