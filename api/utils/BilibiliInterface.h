#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "Util.h"

#pragma once

#define WRONG_MID (-1LL)

#define BILIBILI_VIDEO_ROUTER "video"
#define BILIBILI_MAIN_PAGE_URL "https://www.bilibili.com/"

extern "C" {

#define OUTPUT_DIRECTORY "output"
#ifdef WIN32
    #define OUTPUT_PATH OUTPUT_DIRECTORY "\\crawl_output.json"
#elifdef  __linux__
    #define OUTPUT_PATH OUTPUT_DIRECTORY "/crawl_output.json"
#endif
#define OUTPUT_NAME "crawl_output"

namespace webAPI {
    class Video {
    private:
        long long _publishTime;
        Json json;
        string _title;
        string _author;
        string _description;
        long long _mid;
        string _url;
        string _duration;
        string _image;
        string _string_publishTime;
        unsigned int _views;
        unsigned int _popups;

        enum class JsonSource {
            Raw,
            Feedback
        };

        explicit Video(const Json& json);

        explicit Video(const Json& json, JsonSource source);
    public:
        API static Video fromData(const dataStore::Data &data);

        API static Video fromJson(const Json& json);

        API static Video fromFeedbackJson(const Json& json);

        API static Video fromCompatibleJson(const Json& json);

        API static dataStore::Data toData(const Video &video);

        API static string getVideoURLFromJson(const Json& json);

        API static string getImageURLFromJson(const Json& json);

        [[nodiscard]] API long long const& publishTime() const;

        [[nodiscard]] API dataStore::Data getData() const;

        [[nodiscard]] API Json const& getJson() const;

        [[nodiscard]] API const char* title() const;

        [[nodiscard]] API const char* author() const;

        [[nodiscard]] API const char* description() const;

        [[nodiscard]] API long long const& mid() const;

        [[nodiscard]] API const char* url() const;

        [[nodiscard]] API const char* duration() const;

        [[nodiscard]] API const char* image() const;

        [[nodiscard]] API const char* string_PublishTime() const;

        [[nodiscard]] API unsigned int const& views() const;

        [[nodiscard]] API unsigned int const& popups() const;

        API void write_necessary(Json& json) const;

        API void write_all(Json& json) const;

        API void reset();

        API void format();

        bool operator==(const Video& video) const noexcept {
            return _title == video._title && _author == video._author;
        }
    };

    API void setVideo(Nullable const Video* video);

    API const Video* nowVideo() noexcept(false);

    API void clearVideo();

    API void keepVideo(const Video& video,const char* label,const char* platform);

    API bool enoughVideo(const char* label = nullptr,const char* platform = nullptr);

    API bool duplicateVideo(const Video& video,const char* label,const char* platform = nullptr);
}

}

namespace nlohmann {
    template <>
    struct adl_serializer<webAPI::Video> {
        static webAPI::Video from_json(const json& json) {
            return webAPI::Video::fromCompatibleJson(json);
        }

        static void to_json(json& json,const webAPI::Video& video) {
            video.write_necessary(json);
        }
    };
}

namespace webAPI{
    API map<pair<string,string>,vector<Video>> getVideos();

    API void saveVideos();

    inline Json getVideoJson() {
        Json json;
        for(const auto& group : getVideos())
            for(int i = 0;i < group.second.size();i++) {
                group.second[i].write_necessary(json[group.first.first][i]);
            }
        return json;
    }
}
