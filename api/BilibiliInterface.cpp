#include "BilibiliInterface.h"
#include "pluginInterface.h"

string toReadableTime(long long publishTime){
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

namespace bilibili{
    const Video* _nowVideo;

    Video::Video(const dataStore::Data &data) {
        Json j;
        j = data;
        new (this) Video(j);
    }

    Video::Video(const Json &json) {
        this -> json = json;
        #ifdef DEVELOP
        try {
        #endif
            _publishTime = getPublishTime(json);
            _title = getTitle(json);
            _author = getAuthor(json);
            _description = getDescription(json);
            _mid = getMid(json);
            _url = getVideoURL(json);
            _duration = getVideoDuration(json);
            _image = getImageURL(json);
            _string_publishTime = toReadableTime(publishTime());
        #ifdef DEVELOP
        }catch (exception e){
            warn("Invalid json format ! Now json content :");
            warn(to_string(json).c_str());
            warn(e.what());
            reset();
        }
        #endif
    }

    Video Video::fromData(const dataStore::Data &data) {
        return Video{data};
    }

    Video Video::fromJson(const Json &json) {
        return Video{json};
    }

    dataStore::Data Video::toData(const bilibili::Video &video) {
        return video.json.get<dataStore::Data>();
    }

    string Video::getVideoURLFromJson(const Json &json) {
        if(json.contains("arcurl"))
            return json["arcurl"].get<std::string>();
        if(json.contains("three_point")){
            return json["three_point"][1]["short_link"];
        }
        string error("Invalid Json Format !!! Json :\n");
        error += to_string(json);
        throwError(error.c_str());
        return "";
    }

    string Video::getImageURLFromJson(const Json &json) {
        if(json.contains("pic")){
            string back = json["pic"].get<string>();
            if(startWith(back.c_str(),"https:"))
                return back;
            return "https:" + back;
        }
        string error("Invalid Json Format !!! Json :\n");
        error += to_string(json);
        throwError(error.c_str());
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

    void Video::write_necessary(Json& json) const{
        json["title"] = _title;
        json["publishTime"] = string_PublishTime();
//        json["ctime"] = publishTime();
        json["author"] = _author;
        json["description"] = _description;
//        json["mid"] = _mid;
        json["url"] = _url;
        json["duration"] = _duration;
        json["image"] = _image;
    }

    void Video::write_all(Json& json) const{
        json = getJson();
    }

    void Video::reset() {
        _title.clear();
        _publishTime = -1;
        _author.clear();
        _description.clear();
        _mid = -1;
        _url.clear();
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

    map<string,vector<Video>> videos = map<string,vector<Video>>();

    void keepVideo(const Video& video,const char* label){
        string name(label);
        videos[name].emplace_back(video);
    }

    map<string,vector<Video>> getVideos(){
        return videos;
    }

    bool enoughVideo(const char* label){
        return videos[string(label)].size() >= crawlTask::nowTask() -> videoCount;
    }

    void saveVideos(){
        if(fileExists(OUTPUT_PATH)){
            deleteConfig(OUTPUT_PATH,true);
        }
        Json json;
        for(const auto& group : getVideos())
            for(int i = 0;i < group.second.size();i++) {
                group.second[i].write_necessary(json[group.first][i]);
                #ifdef DEVELOP
                    group.second[i].write_all(json[group.first][i]["all_json"]);
                #endif
            }
        if(storeJson(OUTPUT_NAME,OUTPUT_PATH,json)) {
            saveToFile(OUTPUT_NAME,OUTPUT_PATH);
            storeJson(OUTPUT_NAME,OUTPUT_PATH, nullptr, true);
        }else throwError("Save Output file failed !");
    }
}
