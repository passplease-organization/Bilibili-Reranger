#include "../utils/BilibiliInterface.h"

#include <initializer_list>
#include <regex>
#include <cpr/api.h>

#include "../pluginInterface.h"

string toReadableTime(long long publishTime){// TODO 时间解析有问题
    auto pubTime = static_cast<std::time_t>(publishTime);
    std::time_t current = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    double seconds = std::difftime(current,pubTime);
    auto days = static_cast<long long>(seconds / 86400.0);
    std::stringstream stream;
    if(days > 1){
        if(days > 365){
            stream << std::put_time(std::localtime(&pubTime), "%F");
        }else {
            stream << std::put_time(std::localtime(&pubTime), "%M-%D");
        }
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
    bool hasAnyField(const Json& json,const initializer_list<const char*> fields) {
        for (const auto* field : fields)
            if (json.contains(field))
                return true;
        return false;
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
        requireAnyField(json, {"created", "pubdate", "senddate", "ctime"}, "created/pubdate/senddate/ctime", missing);
        requireField(json, "title", missing);
        requireField(json, "author", missing);
        requireField(json, "description", missing);
        requireField(json, "mid", missing);
        requireAnyField(json, {"bvid", "arcurl"}, "bvid/arcurl", missing);
        requireAnyField(json, {"length", "duration"}, "length/duration", missing);
        requireField(json, "pic", missing);
        requireField(json, "play", missing);
        requireField(json, "video_review", missing);

        if (!missing.empty())
            cppUtil::throwError("Invalid Bilibili video json. Missing fields: ", missing, ". Now json content: ", json.dump());
    }
#endif

    long long getPublishTimeFromJson(const Json& json) {
        if (json.contains("created"))
            return json["created"].get<long long>();
        if (json.contains("pubdate"))
            return json["pubdate"].get<long long>();
        if (json.contains("senddate"))
            return json["senddate"].get<long long>();
        return json["ctime"].get<long long>();
    }

    string getVideoDurationFromJson(const Json& json) {
        if (json.contains("length"))
            return json["length"].get<string>();
        return json["duration"].get<string>();
    }
}

namespace webAPI{
    thread_local const Video* _nowVideo;

    Video::Video(const Json &json) {
        this -> json = json;
#ifdef DEVELOP
        validateVideoJsonFields(json);
#endif
        _publishTime = getPublishTimeFromJson(json);
        _title = json["title"].get<std::string>();
        _author = json["author"].get<std::string>();
        _description = json["description"].get<std::string>();
        _mid = json["mid"].get<int>();
        _url = getVideoURLFromJson(json);
        _duration = getVideoDurationFromJson(json);
        _image = getImageURLFromJson(json);
        _string_publishTime = toReadableTime(publishTime());
        _views = json["play"].get<unsigned int>();
        _popups = json["video_review"].get<unsigned int>();
        format();
    }

    Video Video::fromData(const dataStore::Data &data) {
        return Video{data};
    }

    Video Video::fromJson(const Json &json) {
        return Video{json};
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

    long long const& Video::publishTime() const{
        return _publishTime;
    }

    const char* Video::author() const{
        return _author.c_str();
    }

    const char* Video::description() const{
        return _description.c_str();
    }

    int const& Video::mid() const{
        return _mid;
    }

    dataStore::Data Video::getData() const {
        return toData(*this);
    }

    Json const& Video::getJson() const{
        return json;
    }

    const char* Video::title() const{
        return _title.c_str();
    }

    const char* Video::url() const {
        return _url.c_str();
    }

    const char* Video::duration() const {
        return _duration.c_str();
    }

    const char* Video::image() const {
        return _image.c_str();
    }

    const char* Video::string_PublishTime() const {
        return _string_publishTime.c_str();
    }

    unsigned int const &Video::views() const {
        return _views;
    }

    unsigned int const &Video::popups() const {
        return _popups;
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

    void setVideo(Nullable const Video* video){
        _nowVideo = video;
    }

    const Video* nowVideo(){
        return _nowVideo;
    }

    void clearVideo(){
        setVideo(nullptr);
    }

    thread_local map<pair<string,string>,vector<Video>> videos = map<pair<string,string>,vector<Video>>();

    void keepVideo(const Video& video,const char* label,const char* platform){
        string name(label == nullptr ? "" : label);
        string plat(platform == nullptr ? "" : platform);
        videos[{name,plat}].emplace_back(video);
    }

    map<pair<string,string>,vector<Video>> getVideos(){
        return videos;
    }

    bool enoughVideo(const char* label,const char* platform){
        string name(label == nullptr ? crawlTask::getGroup() -> name : label);
        string plat(platform == nullptr ? crawlTask::getGroup() -> platform : platform);
        return videos[{name,plat}].size() >= crawlTask::nowTask() -> videoCount;
    }

    bool duplicateVideo(const Video& video,const char* label,const char* platform) {
        return std::ranges::any_of(videos[{label,platform}],[video](const auto& oldVideo) {
            return oldVideo == video;
        });
    }

    void saveVideos(){
        Json json;
        for(const auto& group : getVideos())
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
}
