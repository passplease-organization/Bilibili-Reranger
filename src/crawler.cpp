#include <regex>
#include <iostream>
#include <sstream>
#include "Crawler.h"
#include "config.h"
#include "bilibiliAPIs.h"
#include "BilibiliInterface.h"
#include "webAPIs/crawler.h"

#include "platforms/bilibiliLogin.h"
#include "subFeatures/requestHelper.h"
#include "webAPIs/postgres.h"
#if NEED_PORT
    #include "PortListener.h"
    #include <boost/asio.hpp>
    #include <utility>
#endif

CrawlerHelper::CrawlerHelper(){
#if NEED_PORT
    if (crawlInfo != nullptr && crawlInfo -> client != nullptr) {
        const auto* handler = crawlInfo -> client -> handler();
        if (handler != nullptr) {
            subscribers = handler -> subscribers();
            return;
        }
    }
    subscribers = dataStore::Data();
#endif
}

CrawlerHelper::CrawlerHelper(dataStore::Data subscribers)
    : subscribers(std::move(subscribers)) {}

void CrawlerHelper::curlSetup(const string &cookie,const string& useragent){
    CurlHelper::curlSetup();
    curl_easy_setopt(curl,CURLOPT_COOKIE,cookie.c_str());
    curl_easy_setopt(curl,CURLOPT_USERAGENT,useragent.c_str());
    curl_easy_setopt(curl,CURLOPT_REFERER,"https://bilibili.com/");
}

void CrawlerHelper::curlSetup(){
    CurlHelper::curlSetup();
    #if NEED_PORT
        if (crawlInfo == nullptr || crawlInfo -> client == nullptr) {
            warn("Client not initialized, cannot setup curl");
            return;
        }
        const auto* handler = crawlInfo->client->handler();
        if (handler == nullptr) {
            warn("Client handler not initialized, call /set or /login first");
            return;
        }
        curlSetup(handler -> getCOOKIE(), user_agent);
    #else
        curlSetup(cookie,user_agent);
    #endif
}

bool CrawlerHelper::connect(bool deal){
    if(nextURL().empty())
        return false;

    #if MORE_DETAILS
        say("下一次的URL:", false);
        say(url.c_str(), true, GREEN);
    #endif

    clear();
    #if CONNECT_INTERNET
        if(crawlNext())
            CurlHelper::connect(false);
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
        try {
            if (deal && dealJson()) {
                clear();
                return true;
            }
            return !deal;
        }catch (exception& e){
    #if DEVELOP
            warn("Dealing Json encounters problem !");
            say("Json content: ",false,RED);
            say(to_string(json).c_str(),true,RED);
            say("Now url: ",false,RED);
            say(url.c_str(),true,RED);
            throwError(e.what());
    #endif
            say("Please Check COOKIE ! May COOKIE wrong ! Now Client ID :",false);
            say(crawlInfo -> clientId.c_str());
            return false;
        }
}

bool CrawlerHelper::dealJson() {
    #if MORE_DETAILS
        say("爬取中");
    #endif

    if(!tempData.empty()) {
        if(pluginDealJson(tempData))
            return true;

        try {
            json = Json::parse(tempData);
    #ifdef DEVELOP
            auto data = json.get<dataStore::Data>();
            data.setPath(tempDataPath);
            data.setName(tempDataName);
            say("保存此次爬取临时数据");
            data.writeToJson();
    #endif
        }catch (exception& e){
    #ifdef DEVELOP
            warn("爬取数据格式错误！");
            warn("数据如下：");
            warn(tempData.c_str());
    #endif
            const auto* handler = (crawlInfo != nullptr && crawlInfo -> client != nullptr)
                ? crawlInfo -> client -> handler()
                : nullptr;
            if (handler != nullptr && !handler -> validCOOKIE()) {
                warn("请检查COOKIE是否正常，客户端ID: ",false);
                warn(crawlInfo -> clientId.c_str());
            }
            throwError(e.what());
        }
    }
    auto task = crawlTask::nowTask();
    #if NEED_PORT
        if (crawlInfo == nullptr || crawlInfo -> client == nullptr) {
            warn("Client not initialized, cannot deal json");
            return false;
        }
        auto* handler = crawlInfo -> client -> handler();
        if (handler == nullptr) {
            warn("Client handler not initialized, call /set or /login first");
            return false;
        }
        return handler -> dealJson(*this, json, task);
    #else
        switch (task -> mode) {
            case crawlTask::WorkingMode::SUBSCRIBE : {
                if(!startWith(url.c_str(),videoByUser)) {

                #if MORE_DETAILS
                    say("当前搜索的关注用户名：", false);
                    say(task->keyword);
                #endif

                    Json json_subs = getSubscribers();
                    for (auto &up: _getSubscribers(json_subs, false).items()) {

                    #if MORE_DETAILS
                        say("当前比对up名：", false);
                        say(_getSubscriberName(up).c_str(), true, YELLOW);
                    #endif

                        if (_getSubscriberName(up) == task->keyword) {
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
                    const auto* group = crawlTask::getGroup();
                    const char* groupName = group == nullptr ? "" : group -> name;
                    const char* groupPlatform = group == nullptr ? "" : group -> platform;
                    forEachVideo(json,ofPerson){
                        const auto& video = webAPI::Video::fromJson(videoData);
                        webAPI::setVideo(&video);
                        if(roughCheckVideo() && finalCheckVideo())
                            webAPI::keepVideo(video,groupName,groupPlatform);
                        webAPI::clearVideo();
                    }
                    clearURL();
                    return crawlTask::nextTask(true) != nullptr;
                }
                return false;
            }
            case crawlTask::WorkingMode::TAG : {
                const auto* group = crawlTask::getGroup();
                const char* groupName = group == nullptr ? "" : group -> name;
                const char* groupPlatform = group == nullptr ? "" : group -> platform;
                forEachVideo(json,ofSearch){
                    const auto& video = webAPI::Video::fromJson(videoData);
                    if(videoData["tag"].get<string>().find(task -> keyword) == string::npos)
                        continue;
                    webAPI::setVideo(&video);
                    if(roughCheckVideo() && finalCheckVideo())
                        webAPI::keepVideo(video,groupName,groupPlatform);
                    webAPI::clearVideo();
                }
                if(webAPI::enoughVideo(groupName,groupPlatform)) {
                    clearURL();
                    return crawlTask::nextTask(true) != nullptr;
                }
                cout << getDataFromJson(json)["next"] << endl;
                int page = getDataFromJson(json)["next"].get<int>();
                nextPage(page);
                return true;
            }case crawlTask::WorkingMode::SEARCH : {
                const auto* group = crawlTask::getGroup();
                const char* groupName = group == nullptr ? "" : group -> name;
                const char* groupPlatform = group == nullptr ? "" : group -> platform;
                forEachVideo(json,ofSearch){
                    const auto& video = webAPI::Video::fromJson(videoData);
                    webAPI::setVideo(&video);
                    if(roughCheckVideo() && finalCheckVideo())
                        webAPI::keepVideo(video,groupName,groupPlatform);
                    webAPI::clearVideo();
                }
                if(webAPI::enoughVideo(groupName,groupPlatform)) {
                    clearURL();
                    return crawlTask::nextTask(true) != nullptr;
                }
                int page = getDataFromJson(json)["next"].get<int>();
                nextPage(page);
                return true;
            }
            default: return false;
        }
    #endif
}

bool CrawlerHelper::nextPage() {
    auto task = crawlTask::nowTask();
    switch(task -> mode){
        default : return false;
    }
}

unsigned int CrawlerHelper::getPages(const string &url) {
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

void CrawlerHelper::nextPage(unsigned int nowPage) {
    string pn = "pn=";
    string now = pn + std::to_string(nowPage++);
    string next = pn + std::to_string(nowPage);
    size_t pos = url.find(now);
    if(pos != string::npos){
        url.replace(pos,now.length(),next);
    }else url = url.append("&").append(next);
}

void CrawlerHelper::nextMustCrawl() {
    _crawlNext = true;
}

bool CrawlerHelper::finishCrawl() const {
    return url.empty() && crawlTask::nowTask() == nullptr;
}

void CrawlerHelper::nextSearch(const string &url) {
    if(CrawlerHelper::url.empty())
        CrawlerHelper::url = url;
}

bool CrawlerHelper::refreshSubscribers(const bool force) {
    #if NEED_PORT
        if (crawlInfo == nullptr || crawlInfo -> client == nullptr) {
            warn("Client not initialized, cannot refresh subscribers");
            return false;
        }
        auto* handler = crawlInfo -> client -> handler();
        if (handler == nullptr) {
            warn("Client handler not initialized, call /set or /login first");
            return false;
        }
    #else
        auto* handler = new bilibiliLogin::bilibiliLogin();
    #endif
        return handler -> refreshSubscribers(*this, force);
}

bool CrawlerHelper::crawlNext() const {
    return _crawlNext || (crawlTask::nowTask() -> mode != crawlTask::WorkingMode::SUBSCRIBE);
}

void CrawlerHelper::addSubscriber(const dataStore::Data& _subscribers) {
    subscribers += _subscribers;
}

void CrawlerHelper::clearSubscriber() {
    subscribers.clear();
}

dataStore::Data CrawlerHelper::getSubscribers(const string& name) {
    if (name.empty())
        return subscribers;
    Json sub = subscribers;
    for (auto &up: _getSubscribers(sub, false).items()) {
        if (_getSubscriberName(up) == name)
            return up.value().get<dataStore::Data>();
    }
    return dataStore::Data();
}

#if NEED_PORT
    #define CLIENT_COOKIE (crawlInfo -> client -> handler() -> getCOOKIE().c_str())
    webAPI::DbConfig postgresConfig;
    webAPI::postgres dataBase(postgresConfig);
#else
    string cookie;
#endif
string user_agent;

#if NEED_PORT
bool crawl(const std::atomic<bool>& cancel,boost::asio::ip::tcp::socket& socket){
#else
bool crawl(const std::atomic<bool>& cancel){
#endif
    CrawlerHelper helper = CrawlerHelper();
    helper.curlSetup();
    #if NEED_PORT
        dealParams(helper);
    #else
        helper.refreshSubscribers();
    #endif
    const int max_count = config<int>(MAX_CRAWL_COUNT);
    int count = 0;
    do{
        count++;

    #if SLEEP_CRAWL
        say("爬取等待...");
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
        const bool& back = helper.finishCrawl();
        return back && sendMessage(socket,"",!back,false);
    #else
        webAPI::saveVideos();
        return helper.finishCrawl();
#endif
}

string getURL(const crawlTask::Task* task){
    auto url = pluginGetURL();
    if(!url.empty())
        return url;
    #if NEED_PORT
        if (crawlInfo == nullptr || crawlInfo -> client == nullptr) {
            warn("Client not initialized, cannot get url");
            return "";
        }
        auto* handler = crawlInfo -> client -> handler();
        if (handler == nullptr) {
            warn("Client handler not initialized, call /set or /login first");
            return "";
        }
        return handler -> getURL(task);
    #else
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
    #endif
}

#define LOG_ENV(NAME) \
    string err = "未找到环境变量: "; \
    err += (NAME); \
    warn(err.c_str()); \
    error |= true;

bool checkEnv(){
    bool error = false;
#if NEED_PORT
    if (getenv(POSTGRES_SCHEMA) == nullptr || getenv(POSTGRES_USER) == nullptr || getenv(POSTGRES_PASSWORD) == nullptr || getenv(POSTGRES_HOST) == nullptr || getenv(POSTGRES_PORT) == nullptr) {
        LOG_ENV(POSTGRES_SCHEMA "或" POSTGRES_USER "或" POSTGRES_PASSWORD "或" POSTGRES_HOST "或" POSTGRES_PORT);
    }else {
        postgresConfig = webAPI::DbConfig(getenv(POSTGRES_HOST),stoi(getenv(POSTGRES_PORT)),getenv(POSTGRES_SCHEMA),getenv(POSTGRES_USER),getenv(POSTGRES_PASSWORD));
        dataBase = webAPI::postgres(postgresConfig);
        say("数据库初始化");
        error |= !dataBase.init();
        if (error)
            warn("数据库初始化失败！");
        else say("数据库初始化完成");
    }
#else
    if(cookie.empty()){
        if (getenv(COOKIE) == nullptr){
            string err = "未找到环境变量: ";
            err += COOKIE;
            warn(err.c_str());
            error |= true;
        }else cookie = getenv(COOKIE);
    }
#endif
    if(user_agent.empty()){
        if (getenv(USERAGENT) == nullptr){
            LOG_ENV(USERAGENT);
        }else user_agent = getenv(USERAGENT);
    }
    return error;
}
