#include "../utils/BilibiliInterface.h"

#include <ctime>
#include <iomanip>
#include <initializer_list>
#include <regex>
#include <cpr/api.h>

#include "../pluginInterface.h"

string toReadableTime(long long publishTime){
    const auto pubTime = static_cast<std::time_t>(publishTime);
    const std::time_t current = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    const double seconds = std::difftime(current,pubTime);
    const auto days = static_cast<long long>(seconds / 86400.0);
    if(days > 1){
        std::stringstream stream;
        stream << std::put_time(std::localtime(&pubTime), "%F");
        return stream.str();
    }
    if(days == 1){
        return "昨天";
    }
    auto hours = static_cast<long long>(seconds / 3600.0);
    if(hours > 1){
        return to_string(hours) + "小时前";
    }
    auto second = static_cast<long long>(seconds);
    if(second > 0)
        return to_string(second) + "秒前";
    return "刚刚";
}

namespace {
    string formatDurationFromSeconds(const long long duration) {
        const auto hours = duration / 3600;
        const auto minutes = duration % 3600 / 60;
        const auto seconds = duration % 60;

        std::stringstream stream;
        if (hours > 0) {
            stream << hours << ":"
                   << std::setw(2) << std::setfill('0') << minutes << ":";
        }else {
            stream << minutes << ":";
        }
        stream << std::setw(2) << std::setfill('0') << seconds;
        return stream.str();
    }

    bool hasAnyField(const Json& json,const initializer_list<const char*> fields) {
        for (const auto* field : fields)
            if (json.contains(field))
                return true;
        return false;
    }

    bool missingOrNull(const Json& json, const char* field) {
        return !json.contains(field) || json[field].is_null();
    }

    // Fallback for "views"/"play"-like numeric fields: null, missing or
    // wrongly-typed values become 0 instead of throwing type_error.302.
    unsigned int getUnsignedIntOrZero(const Json& json, const char* field) {
        if (missingOrNull(json, field))
            return 0;
        const auto& value = json[field];
        if (value.is_number_unsigned())
            return value.get<unsigned int>();
        if (value.is_number_integer()) {
            const long long number = value.get<long long>();
            return number > 0 ? static_cast<unsigned int>(number) : 0;
        }
        if (value.is_number_float()) {
            const double number = value.get<double>();
            return number > 0 ? static_cast<unsigned int>(number) : 0;
        }
        if (value.is_string()) {
            try {
                const auto number = std::stoull(value.get<string>());
                return static_cast<unsigned int>(number);
            } catch (const std::exception&) {
                return 0;
            }
        }
        return 0;
    }

    Json normalizeVideoJson(Json json) {
        if (!json.is_object())
            return json;

        if (json.contains("owner") && json["owner"].is_object()) {
            const auto& owner = json["owner"];
            if (missingOrNull(json, "author") && owner.contains("name"))
                json["author"] = owner["name"];
            if (missingOrNull(json, "mid") && owner.contains("mid"))
                json["mid"] = owner["mid"];
        }

        if (json.contains("author") && json["author"].is_object()) {
            const auto& author = json["author"];
            if (author.contains("name"))
                json["author"] = author["name"];
            if (missingOrNull(json, "mid") && author.contains("mid"))
                json["mid"] = author["mid"];
        }

        if (missingOrNull(json, "pic") && json.contains("cover"))
            json["pic"] = json["cover"];

        if (missingOrNull(json, "description"))
            json["description"] = "";

        if (json.contains("stat") && json["stat"].is_object()) {
            const auto& stat = json["stat"];
            if (missingOrNull(json, "play") && stat.contains("view") && stat["view"].is_number())
                json["play"] = stat["view"];
            if (missingOrNull(json, "video_review") && stat.contains("danmaku") && stat["danmaku"].is_number())
                json["video_review"] = stat["danmaku"];
        }

        // Fallback: normalize null/missing numeric fields to 0
        if (missingOrNull(json, "play"))
            json["play"] = 0;
        if (missingOrNull(json, "video_review"))
            json["video_review"] = 0;

        if (missingOrNull(json, "length") && json.contains("duration")) {
            const auto& duration = json["duration"];
            if (duration.is_number_integer() || duration.is_number_unsigned())
                json["length"] = formatDurationFromSeconds(duration.get<long long>());
            else if (duration.is_string())
                json["length"] = duration;
        }

        if (missingOrNull(json, "arcurl") && json.contains("uri"))
            json["arcurl"] = json["uri"];

        return json;
    }

    bool isFeedbackVideoJson(const Json& json) {
        return json.is_object() && hasAnyField(json, {
            "videoTime", "videoURL", "views", "popups"
        });
    }

#ifdef DEVELOP
    void addMissingField(string& missing,const string& field) {
        if (!missing.empty())
            missing += ", ";
        missing += field;
    }

    void requireField(const Json& json,const char* field,string& missing) {
        if (!json.contains(field))
            addMissingField(missing, field);
    }

    void requireAnyField(const Json& json,const initializer_list<const char*> fields,const char* name,string& missing) {
        if (!hasAnyField(json, fields))
            addMissingField(missing, name);
    }

    void validateVideoJsonFields(const Json& json) {
        if (!json.is_object())
            cppUtil::throwError("Invalid Bilibili video json. Expected object, now json content: ", json.dump());

        string missing;
        requireField(json, "title", missing);
        requireField(json, "author", missing);
        requireField(json, "description", missing);
        requireAnyField(json, {"bvid", "arcurl"}, "bvid/arcurl", missing);
        requireAnyField(json, {"length", "duration"}, "length/duration", missing);
        requireField(json, "pic", missing);
        requireField(json, "play", missing);
        requireField(json, "video_review", missing);

        if (!missing.empty())
            cppUtil::throwError("Invalid Bilibili video json. Missing fields: ", missing, ". Now json content: ", json.dump());
    }

    void validateFeedbackVideoJsonFields(const Json& json) {
        if (!json.is_object())
            cppUtil::throwError("Invalid feedback video json. Expected object, now json content: ", json.dump());

        string missing;
        requireField(json, "title", missing);
        requireField(json, "author", missing);
        requireField(json, "url", missing);
        requireField(json, "videoTime", missing);
        requireField(json, "videoURL", missing);
        requireField(json, "views", missing);
        requireField(json, "popups", missing);

        if (!missing.empty())
            cppUtil::throwError("Invalid feedback video json. Missing fields: ", missing, ". Now json content: ", json.dump());
    }
#endif

    long long getPublishTimeFromJson(const Json& json) {
        if (json.contains("created") && json["created"].is_number())
            return json["created"].get<long long>();
        if (json.contains("pubdate") && json["pubdate"].is_number())
            return json["pubdate"].get<long long>();
        if (json.contains("senddate") && json["senddate"].is_number())
            return json["senddate"].get<long long>();
        if (json.contains("ctime") && json["ctime"].is_number())
            return json["ctime"].get<long long>();
        return -1;
    }

    string getVideoDurationFromJson(const Json& json) {
        if (json.contains("length")) {
            const auto& length = json["length"];
            if (length.is_string())
                return length.get<string>();
            if (length.is_number_integer() || length.is_number_unsigned())
                return formatDurationFromSeconds(length.get<long long>());
        }
        if (json.contains("duration")) {
            const auto& duration = json["duration"];
            if (duration.is_string())
                return duration.get<string>();
            if (duration.is_number_integer() || duration.is_number_unsigned())
                return formatDurationFromSeconds(duration.get<long long>());
        }
        string error("Invalid Json Format !!! Json :\n");
        error += to_string(json);
        cppUtil::throwError(error);
        return "";
    }

    string getFeedbackPublishTimeStringFromJson(const Json& json) {
        if (!json.contains("publishTime") || json["publishTime"].is_null())
            return "";
        if (json["publishTime"].is_string())
            return json["publishTime"].get<string>();
        if (json["publishTime"].is_number_integer() || json["publishTime"].is_number_unsigned())
            return toReadableTime(json["publishTime"].get<long long>());
        return json["publishTime"].dump();
    }

    long long getFeedbackPublishTimeFromJson(const Json& json) {
        if (!json.contains("publishTime") || json["publishTime"].is_null())
            return -1;
        if (json["publishTime"].is_number_integer() || json["publishTime"].is_number_unsigned())
            return json["publishTime"].get<long long>();
        if (!json["publishTime"].is_string())
            return -1;

        const auto value = json["publishTime"].get<string>();
        std::tm time{};
        std::istringstream stream(value);
        stream >> std::get_time(&time, "%Y-%m-%d");
        if (stream.fail())
            return -1;
        return static_cast<long long>(std::mktime(&time));
    }
}

namespace webAPI{
    thread_local const Video* _nowVideo;

    Video::Video(const Json &json): Video(json, JsonSource::Raw) {}

    Video::Video(const Json &json, const JsonSource source) {
        if (source == JsonSource::Feedback) {
            this -> json = json;
#ifdef DEVELOP
            validateFeedbackVideoJsonFields(this -> json);
#endif
            _publishTime = getFeedbackPublishTimeFromJson(this -> json);
            _title = this -> json["title"].get<std::string>();
            _author = this -> json["author"].get<std::string>();
            _description = this -> json.value("description", std::string());
            _mid = WRONG_MID;
            _url = this -> json["url"].get<std::string>();
            _duration = this -> json["videoTime"].get<std::string>();
            _image = this -> json["videoURL"].get<std::string>();
            _string_publishTime = getFeedbackPublishTimeStringFromJson(this -> json);
            _views = getUnsignedIntOrZero(this -> json, "views");
            _popups = getUnsignedIntOrZero(this -> json, "popups");
            // normalize null data to 0 so the stored json stays consistent
            this -> json["views"] = _views;
            this -> json["popups"] = _popups;
            format();
            return;
        }

        this -> json = normalizeVideoJson(json);
#ifdef DEVELOP
        validateVideoJsonFields(this -> json);
#endif
        _publishTime = getPublishTimeFromJson(this -> json);
        _title = this -> json["title"].get<std::string>();
        _author = this -> json["author"].get<std::string>();
        _description = this -> json["description"].get<std::string>();
        _mid = (missingOrNull(this -> json, "mid") || !this -> json["mid"].is_number())
                   ? WRONG_MID : this -> json["mid"].get<long long>();
        _url = getVideoURLFromJson(this -> json);
        _duration = getVideoDurationFromJson(this -> json);
        _image = getImageURLFromJson(this -> json);
        _string_publishTime = publishTime() >= 0 ? toReadableTime(publishTime()) : "";
        _views = getUnsignedIntOrZero(this -> json, "play");
        _popups = getUnsignedIntOrZero(this -> json, "video_review");
        format();
    }

    Video Video::fromData(const dataStore::Data &data) {
        return Video{data};
    }

    Video Video::fromJson(const Json &json) {
        return Video{json};
    }

    Video Video::fromFeedbackJson(const Json &json) {
        return Video{json, JsonSource::Feedback};
    }

    Video Video::fromCompatibleJson(const Json &json) {
        if (isFeedbackVideoJson(json))
            return fromFeedbackJson(json);
        return fromJson(json);
    }

    Video Video::fromCompatibleJson(const Json &json, unsigned short int recommendTimes) {
        Video video = fromCompatibleJson(json);
        video._recommendTimes = recommendTimes;
        return video;
    }

    dataStore::Data Video::toData(const webAPI::Video &video) {
        return video.json;
    }

    string Video::getVideoURLFromJson(const Json &json) {
        if (json.contains("bvid"))
            return BILIBILI_MAIN_PAGE_URL BILIBILI_VIDEO_ROUTER "/" + json["bvid"].get<string>();
        if(json.contains("arcurl"))
            return json["arcurl"].get<std::string>();
        string error("Invalid Json Format !!! Json :\n");
        error += to_string(json);
        cppUtil::throwError(error);
        return "";
    }

    string Video::getImageURLFromJson(const Json &json) {
        if(json.contains("pic")){
            string back = json["pic"].get<string>();
            if(startWith(back.c_str(),"https:") || startWith(back.c_str(),"http:"))
                return back;
            if(startWith(back.c_str(),"//"))
                return "https:" + back;
            return back;
        }
        string error("Invalid Json Format !!! Json :\n");
        error += to_string(json);
        cppUtil::throwError(error);
        return "";
    }

    void Video::write_necessary(Json& json) const{
        json["title"] = _title;
        json["publishTime"] = string_PublishTime();
//        json["ctime"] = publishTime();
        json["author"] = _author;
        json["description"] = _description;
//        json["mid"] = _mid;
        json["url"] = _url;
        json["videoTime"] = _duration;
        json["videoURL"] = _image;
        json["views"] = _views;
        json["popups"] = _popups;
    }

    void Video::write_all(Json& json) const{
        json = getJson();
    }

    void Video::reset() {
        _title.clear();
        _publishTime = -1;
        _author.clear();
        _description.clear();
        _mid = WRONG_MID;
        _url.clear();
        _duration.clear();
        _image.clear();
        _string_publishTime.clear();
        _views = 0;
    }

    void Video::format() {
        _title = regex_replace(_title,regex("<em[^>]*>|</em>"),"");
    }

    [[deprecated]] void setVideo(Nullable const Video* video){
        _nowVideo = video;
    }

    [[deprecated]] const Video* nowVideo(){
        return _nowVideo;
    }

    [[deprecated]] void clearVideo(){
        setVideo(nullptr);
    }

    [[deprecated]] thread_local map<pair<string,string>,vector<Video>> _videos = map<pair<string,string>,vector<Video>>();

    void keepVideo(const Video& video,const char* label,const char* platform){
        string name(label == nullptr ? "" : label);
        string plat(platform == nullptr ? "" : platform);
        _videos[{name,plat}].emplace_back(video);
    }

    [[deprecated]] map<pair<string,string>,vector<Video>> _getVideos(){
        return _videos;
    }

    [[deprecated]] bool enoughVideo(const char* label,const char* platform){
        string name(label == nullptr ? crawlTask::getGroup() -> name : label);
        string plat(platform == nullptr ? crawlTask::getGroup() -> platform : platform);
        return _videos[{name,plat}].size() >= crawlTask::nowTask() -> videoCount;
    }

    [[deprecated]] bool duplicateVideo(const Video& video,const char* label,const char* platform) {
        return std::ranges::any_of(_videos[{label,platform}],[video](const auto& oldVideo) {
            return oldVideo == video;
        });
    }

    [[deprecated]] void saveVideos(){
        Json json;
        for(const auto& group : _getVideos())
            for(int i = 0;i < group.second.size();i++) {
                group.second[i].write_necessary(json[group.first.second][group.first.first][i]);
                #ifdef DEVELOP
                    group.second[i].write_all(json[group.first.second][group.first.first][i]["all_json"]);
                #endif
            }
        if(storeJson(OUTPUT_NAME,OUTPUT_PATH,json)) {
            saveToFile(OUTPUT_NAME,OUTPUT_PATH);
            storeJson(OUTPUT_NAME,OUTPUT_PATH, nullptr, true);
        }else cppUtil::throwError("Save Output file failed !");
    }

    thread_local vector<Video>* videos = nullptr;

    static string name;

    void setVideosName(const string &_name) {
        name = _name;
        videos = new vector<Video>();
    }

    void rememberVideos(const vector<Video>& videos_) {
        if (videos == nullptr)
            return;
        videos -> insert(videos -> end(),videos_.begin(), videos_.end());
    }

    void rememberVideo(const Video& video) {
        if (videos == nullptr)
            return;
        videos -> push_back(video);
    }

    const vector<Video>& getVideos() {
        return *videos;
    }

    Json getVideoJson() {
        Json json;
        for(int i = 0;i < getVideos().size();i++)
            getVideos()[i].write_necessary(json[i]);
        Json j;
        j[name] = json;
        return j;
    }
}
