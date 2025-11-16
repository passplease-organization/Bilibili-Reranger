#pragma once

#include "Util.h"
#include "pluginInterface.h"
#include "bilibili_wbi/wbi.h"

#pragma once

#define getPublishTime(json) (json.contains("ctime") ? json["ctime"].get<long long>() : json["pubdate"].get<long long>())
#define getTitle(json) json["title"].get<std::string>()
#define getAuthor(json) json["author"].get<std::string>()
#define getDescription(json) (json.contains("description") ? json["description"].get<std::string>() : "")
#define WRONG_MID (-1)
#define getMid(json) (json.contains("mid") ? json["mid"].get<int>() : WRONG_MID)
#define getVideoURL(json) Video::getVideoURLFromJson(json)
#define getVideoDuration(json) json["duration"].get<std::string>()
#define getImageURL(json) Video::getImageURLFromJson(json)

extern "C" {

#ifdef WIN32
    #define OUTPUT_PATH "output\\crawl_output.json"
#elifdef  __linux__
    #define OUTPUT_PATH "output/crawl_output.json"
#endif
#define OUTPUT_NAME "crawl_output"

namespace bilibili {
    class Video {
    private:
        long long _publishTime;
        Json json;
        string _title;
        string _author;
        string _description;
        int _mid;
        string _url;
        string _duration;
        string _image;
        string _string_publishTime;

        explicit Video(const dataStore::Data &data);

        explicit Video(const Json& json);
    public:
        API static Video fromData(const dataStore::Data &data);

        API static Video fromJson(const Json& json);

        API static dataStore::Data toData(const Video &video);

        API static string getVideoURLFromJson(const Json& json);

        API static string getImageURLFromJson(const Json& json);

        [[nodiscard]] API long long const& publishTime() const;

        [[nodiscard]] API dataStore::Data getData() const;

        [[nodiscard]] API Json const& getJson() const;

        [[nodiscard]] API const char* title() const;

        [[nodiscard]] API const char* author() const;

        [[nodiscard]] API const char* description() const;

        [[nodiscard]] API int const& mid() const;

        [[nodiscard]] API const char* url() const;

        [[nodiscard]] API const char* duration() const;

        [[nodiscard]] API const char* image() const;

        [[nodiscard]] API const char* string_PublishTime() const;

        API void write_necessary(Json& json) const;

        API void write_all(Json& json) const;

        API void reset();
    };

    API void setVideo(Nullable const Video* video);

    API const Video* nowVideo() noexcept(false);

    API void clearVideo();

    API void keepVideo(const Video& video,const char* label = crawlTask::getGroup() -> name);

    API bool enoughVideo(const char* label = crawlTask::getGroup() -> name);
}

}

namespace bilibili{
    API map<string,vector<Video>> getVideos();

    API void saveVideos();
}