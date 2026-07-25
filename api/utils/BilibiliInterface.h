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
        unsigned short int _recommendTimes;

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

        API static Video fromCompatibleJson(const Json& json, unsigned short int recommendTimes);

        API static dataStore::Data toData(const Video &video);

        API static string getVideoURLFromJson(const Json& json);

        API static string getImageURLFromJson(const Json& json);

        [[nodiscard]] API long long const& publishTime() const{
            return _publishTime;
        }

        [[nodiscard]] API dataStore::Data getData() const{
            return toData(*this);
        }

        [[nodiscard]] API Json const& getJson() const{
            return json;
        }

        [[nodiscard]] API const char* title() const{
            return _title.c_str();
        }

        [[nodiscard]] API const char* author() const{
            return _author.c_str();
        }

        [[nodiscard]] API const char* description() const{
            return _description.c_str();
        }

        [[nodiscard]] API long long const& mid() const{
            return _mid;
        }

        [[nodiscard]] API const char* url() const{
            return _url.c_str();
        }

        [[nodiscard]] API const char* duration() const{
            return _duration.c_str();
        }

        [[nodiscard]] API const char* image() const{
            return _image.c_str();
        }

        [[nodiscard]] API const char* string_PublishTime() const{
            return _string_publishTime.c_str();
        }

        [[nodiscard]] API unsigned int const& views() const{
            return _views;
        }

        [[nodiscard]] API unsigned int const& popups() const{
            return _popups;
        }

        [[nodiscard]] API unsigned short int const& recommendTimes() const {
            return _recommendTimes;
        }

        API void write_necessary(Json& json) const;

        API void write_all(Json& json) const;

        API void reset();

        API void format();

        bool operator==(const Video& video) const noexcept {
            return _title == video._title && _author == video._author;
        }
    };

    [[deprecated]] API void setVideo(Nullable const Video* video);

    [[deprecated]] API const Video* nowVideo() noexcept(false);

    [[deprecated]] API void clearVideo();

    [[deprecated]] API void keepVideo(const Video& video,const char* label,const char* platform);

    [[deprecated]] API bool enoughVideo(const char* label = nullptr,const char* platform = nullptr);

    [[deprecated]] API bool duplicateVideo(const Video& video,const char* label,const char* platform = nullptr);
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
    [[deprecated]] API map<pair<string,string>,vector<Video>> _getVideos();

    API const vector<Video>& getVideos();

    API void rememberVideos(const vector<Video>& videos);

    API void rememberVideo(const Video& video);

    [[deprecated]] API void saveVideos();

    API void setVideosName(const string& name);

    /**
     * Will commit all recommended videos to database and delete outdate videos
     * @return All Videos json for frontend
     */
    API Json getVideoJson();
}
