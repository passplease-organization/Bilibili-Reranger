
#include <regex>
#include <iostream>
#include <sstream>
#include "Crawler.h"
#include "develop/flags.h"
#include "config.h"
#include "bilibiliAPIs.h"
#include "BilibiliInterface.h"
#if NEED_PORT
    #include "PortListener.h"
#endif

CurlHelper::CurlHelper(){
    curl = curl_easy_init();
}

CurlHelper::~CurlHelper(){
    if(curl != nullptr)
        curl_easy_cleanup(curl);
}

void CurlHelper::clear() {
    json.clear();
    tempData.clear();
}

void CurlHelper::curlSetup(const char *cookie, const char *useragent){
    if(curl == nullptr){
        throwError("创建CURL失败");
        return;
    }
    curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,CurlHelper::saveData);
    curl_easy_setopt(curl,CURLOPT_WRITEDATA,this);
    curl_easy_setopt(curl,CURLOPT_WRITEDATA,this);
    curl_easy_setopt(curl,CURLOPT_COOKIE,cookie);
    curl_easy_setopt(curl,CURLOPT_USERAGENT,useragent);
}

bool CurlHelper::connect(bool deal){
    if(nextURL().empty())
        return false;

    #if MORE_DETAILS
        say("下一次的URL:", false);
        say(url.c_str(), true, GREEN);
    #endif

    curl_easy_setopt(curl,CURLOPT_URL,nextURL().c_str());
    clear();
    #if CONNECT_INTERNET
        if(crawlNext()){
            _crawlNext = false;
            CURLcode code = curl_easy_perform(curl);
            if (CURLE_OK != code) {
                clear();
                warn("连接链接失败，信息如下：", false);
                warn(curl_easy_strerror(code), false);
                warn("错误码：", false);
                warn(to_string(code).c_str());
                return false;
            }
        }
    #else
        if(fileExists(BILIBILI_DATA)) {
            ifstream data(BILIBILI_DATA);
            if(data.is_open()){
                stringstream stream;
                stream << data.rdbuf();
                tempData = stream.str();
            }
        }
    #endif
    #if DEVELOP
        try {
    #endif
            if (deal && dealJson()) {
                clear();
                return true;
            }
            return !deal;
    #if DEVELOP
        }catch (exception e){
            warn("Dealing Json encounters problem !");
            say("Json content: ",false,RED);
            say(to_string(json).c_str(),true,RED);
            say("Now url: ",false,RED);
            say(url.c_str(),true,RED);
            throwError(e.what());
            return false;
        }
    #endif
}

CURL *CurlHelper::getCurl() const{
    return curl;
}

bool CurlHelper::dealJson() {
    #if MORE_DETAILS
        say("爬取中");
    #endif

    if(!tempData.empty()) {
        if(pluginDealJson(tempData))
            return true;

    #ifdef DEVELOP
        try {
    #endif
            json = Json::parse(tempData);
    #ifdef DEVELOP
            auto data = json.get<dataStore::Data>();
            data.setPath(tempDataPath);
            data.setName(tempDataName);
            say("保存此次爬取临时数据");
            data.writeToJson();
        }catch (exception e){
            warn("爬取数据格式错误！");
            warn("数据如下：");
            warn(tempData.c_str());
            throwError(e.what());
        }
    #endif
    }
    auto task = crawlTask::nowTask();
    switch (task -> mode) {
        case crawlTask::WorkingMode::SUBSCRIBE : {
            if(!startWith(url.c_str(),videoByUser)) {

            #if MORE_DETAILS
                say("当前搜索的关注用户名：", false);
                say(task->keyword);
            #endif

                json = subscribers;
                for (auto &up: getSubscribers(json, false).items()) {

                #if MORE_DETAILS
                    say("当前比对up名：", false);
                    say(getSubscriberName(up).c_str(), true, YELLOW);
                #endif

                    if (getSubscriberName(up) == task->keyword) {
                        clearURL();
                        string url(videoByUser);
                        url += "?vmid=";
                        url += to_string(up.value().at(VMID).get<int>());
                        url += "&ps=";
                        url += std::to_string(config<int>(SUBSCRIBE_SEARCH_VIDEO_COUNT));
                        nextSearch(url);

                    #if MORE_DETAILS
                        say("关注用户爬取一次", true, BLUE);
                    #endif

                        nextMustCrawl();
                        return true;
                    }
                }
            }else{
                forEachVideo(json,ofPerson){
                    const auto& video = bilibili::Video::fromJson(videoData);
                    bilibili::setVideo(&video);
                    if(roughCheckVideo() && finalCheckVideo())
                        bilibili::keepVideo(video);
                    bilibili::clearVideo();
                }
                clearURL();
                return crawlTask::nextTask(true) != nullptr;
            }
            return false;
        }
        case crawlTask::WorkingMode::TAG : {
            forEachVideo(json,ofSearch){
                const auto& video = bilibili::Video::fromJson(videoData);
                if(videoData["tag"].get<string>().find(task -> keyword) == string::npos)
                    continue;
                bilibili::setVideo(&video);
                if(roughCheckVideo() && finalCheckVideo())
                    bilibili::keepVideo(video);
                bilibili::clearVideo();
            }
            if(bilibili::enoughVideo()) {
                clearURL();
                return crawlTask::nextTask(true) != nullptr;
            }
            cout << getDataFromJson(json)["next"] << endl;
            int page = getDataFromJson(json)["next"].get<int>();
            nextPage(page);
            return true;
        }case crawlTask::WorkingMode::SEARCH : {
            forEachVideo(json,ofSearch){
                const auto& video = bilibili::Video::fromJson(videoData);
                bilibili::setVideo(&video);
                if(roughCheckVideo() && finalCheckVideo())
                    bilibili::keepVideo(video);
                bilibili::clearVideo();
            }
            if(bilibili::enoughVideo()) {
                clearURL();
                return crawlTask::nextTask(true) != nullptr;
            }
            int page = getDataFromJson(json)["next"].get<int>();
            nextPage(page);
            return true;
        }
        default: return false;
    }
}

void CurlHelper::dealJson(Json &_json) {
    clear();
    this -> json = _json;
    dealJson();
    clear();
}

size_t CurlHelper::saveData(char *data, size_t size, size_t member, void *userdata) {
    auto* helper = static_cast<CurlHelper*>(userdata);
    long sizes = size * member;
    helper -> tempData += string(data,sizes);
    return sizes;
}

const string &CurlHelper::nextURL() const {
    return url;
}

void CurlHelper::clearURL() {
    url = "";
}

bool CurlHelper::nextPage() {
    auto task = crawlTask::nowTask();
    switch(task -> mode){
        default : return false;
    }
}

unsigned int CurlHelper::getPages(const string &url) {
    regex regular(".*?pn=([0-9]*).*?");
    smatch result;
    if(regex_match(url,result,regular)) {
        try {
            return stoi(result[1].str());
        } catch (const std::invalid_argument& ia) {
            // 捕获异常：当字符串内容无法被解析为数字时（例如 "abc"）
            std::cerr << "Invalid argument: " << ia.what() << '\n';
            // 在这里添加错误处理逻辑，比如设置 back 为一个默认的错误值
            return 1;
        } catch (const std::out_of_range& oor) {
            // 捕获异常：当转换后的数字超出了 int 类型的表示范围时
            std::cerr << "Out of Range error: " << oor.what() << '\n';
            // 添加相应的错误处理逻辑
            return INT_MAX;
        }
    }
    return 1;
}

void CurlHelper::nextPage(unsigned int nowPage) {
    string pn = "pn=";
    string now = pn + std::to_string(nowPage++);
    string next = pn + std::to_string(nowPage);
    size_t pos = url.find(now);
    if(pos != string::npos){
        url.replace(pos,now.length(),next);
    }else url = url.append("&").append(next);
}

void CurlHelper::nextMustCrawl() {
    _crawlNext = true;
}

bool CurlHelper::finishCrawl() const {
    return url.empty() && crawlTask::nowTask() == nullptr;
}

void CurlHelper::nextSearch(const string &url) {
    if(CurlHelper::url.empty())
        CurlHelper::url = url;
}

void CurlHelper::refreshSubscribers(const bool force) {
    #if MORE_DETAILS
        say("开始准备关注博主名单");
    #endif

    if(subscribers.empty() || force) {
        clearURL();
        crawlTask::Task t("", 0, crawlTask::WorkingMode::SUBSCRIBE);
        nextSearch(getURL(&t));
        do{
            nextMustCrawl();
            connect(false);
            json = Json::parse(tempData);

        #ifdef DEVELOP
            dataStore::Data data = json.get<dataStore::Data>();
        #endif

            subscribers += getDataFromJson(json).get<dataStore::Data>();
            int count = getSubscriberCount(json);
            int pages = count / 50 + 1;
            unsigned int nowPage = getPages(url);

        #if MORE_DETAILS
            say("关注博主刷新完成第",false);
            say(to_string(nowPage).c_str(),false);
            say("页");
        #endif

            if(nowPage >= pages)
                break;
            nextPage(nowPage);
        }while(true);
        clear();
        clearURL();
    }

    #if MORE_DETAILS
        say("关注博主名单已准备完成");
    #endif
}

bool CurlHelper::crawlNext() const {
    return _crawlNext || (crawlTask::nowTask() -> mode != crawlTask::WorkingMode::SUBSCRIBE);
}

dataStore::Data CurlHelper::subscribers = dataStore::Data{};

const char* cookie = getenv(COOKIE);
const char* user_agent = getenv(USERAGENT);

bool crawl(const std::atomic<bool>& cancel){
    CurlHelper helper = CurlHelper();
    helper.curlSetup(cookie,user_agent);
    #if NEED_PORT
    #else
        helper.refreshSubscribers();
    #endif
    const int max_count = config<int>(MAX_CRAWL_COUNT);
    int count = 0;
    do{
        count++;

    #if SLEEP_CRAWL
        say("等待中...");
        #ifdef WIN32
            Sleep(config<int>(WAIT_TIME));
        #elifdef __linux__
            sleep(config<int>(WAIT_TIME));
        #endif
    #endif

        const auto task = crawlTask::nowTask();
        if(task == nullptr)
            break;
        helper.nextSearch(getURL(task));
    }while(!cancel && helper.connect() && count < max_count);

    #if NEED_PORT
        sendMessage();
    #else
        bilibili::saveVideos();
    #endif
    return helper.finishCrawl();
}

string getURL(const crawlTask::Task* task){
    auto url = pluginGetURL();
    if(!url.empty())
        return url;
    switch (task -> mode) {
        case crawlTask::WorkingMode::SUBSCRIBE: {
            string back = mySubscribers;
            back += "?vmid=";
            back += config<string>(VMID);
            return back;
        }
        case crawlTask::WorkingMode::TAG :
        case crawlTask::WorkingMode::SEARCH : {
            string back = searchVideos;
            back += "&page_size=";
            back += to_string(config<int>(SEARCH_PAGE_SIZE));
            back += "&keyword=";
            back += task -> keyword;
            return back;
        }
        default: return "";
    }
}

bool checkEnv(){
    bool error = true;
    if(cookie == nullptr){
        string err = "未找到环境变量: ";
        err += COOKIE;
        warn(err.c_str());
        error &= false;
    }
    if(user_agent == nullptr){
        string err = "未找到环境变量: ";
        err += USERAGENT;
        warn(err.c_str());
        error &= false;
    }
    return error;
}