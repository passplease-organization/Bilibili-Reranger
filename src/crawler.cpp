#include <regex>
#include <iostream>
#include "Crawler.h"
#include "../api/utils/config.h"
#include "bilibiliAPIs.h"
#include "utils/BilibiliInterface.h"
#include "webAPIs/crawler.h"

#include "PluginHandler.h"
#include "platforms/bilibiliHandler.h"
#include "subFeatures/requestHelper.h"
#include "webAPIs/postgres.h"
#include "webAPIs/browse.h"
#include "PortListener.h"
#include <boost/asio.hpp>
#include <utility>

CrawlerHelper::CrawlerHelper(){
    if (crawlInfo != nullptr && crawlInfo -> client != nullptr) {
        const auto* handler = crawlInfo -> client -> handler();
        if (handler != nullptr) {
            subscribers = handler -> subscribers();
            return;
        }
    }
    subscribers = dataStore::Data();
}

CrawlerHelper::CrawlerHelper(dataStore::Data subscribers)
    : subscribers(std::move(subscribers)) {}

void CrawlerHelper::curlSetup(const string &cookie,const string& useragent){
    CurlHelper::curlSetup();
    curl_easy_setopt(curl,CURLOPT_COOKIE,cookie.c_str());
    curl_easy_setopt(curl,CURLOPT_USERAGENT,useragent.c_str());
}

void CrawlerHelper::curlSetup(){
    CurlHelper::curlSetup();
    if (crawlInfo == nullptr || crawlInfo -> client == nullptr) {
        cppUtil::warn("Client not initialized, cannot setup curl");
        return;
    }
    const auto* handler = crawlInfo->client->handler();
    if (handler == nullptr) {
        cppUtil::warn("Client handler not initialized, call /set or /login first");
        return;
    }
    curlSetup(handler -> getWorker(nullptr).context->cookie, user_agent);
}

bool CrawlerHelper::connect(bool deal){
    if(nextURL().empty())
        return false;

    #if MORE_DETAILS
        cppUtil::say({false, nullptr}, "下一次的URL:");
        cppUtil::say({true, GREEN}, url);
    #endif

    clear();
    #if ALL_CONTAINER_ONLINE
        if(crawlNext())
            CurlHelper::connect(false);
    #else
        // if(fileExists(BILIBILI_DATA)) {
        //     ifstream data(BILIBILI_DATA);
        //     if(data.is_open()){
        //         stringstream stream;
        //         stream << data.rdbuf();
        //         tempData = stream.str();
        //     }
        // }
    #endif
        try {
            if (deal && dealJson()) {
                clear();
                return true;
            }
            return !deal;
        }catch (exception& e){
    #if DEVELOP
            cppUtil::warn("Dealing Json encounters problem !");
            cppUtil::say({false, RED}, "Json content: ");
            cppUtil::say({true, RED}, json);
            cppUtil::say({false, RED}, "Now url: ");
            cppUtil::say({true, RED}, url);
            cppUtil::throwError(e.what());
    #endif
            cppUtil::say({false, nullptr}, "Please Check COOKIE ! May COOKIE wrong ! Now Client ID :");
            cppUtil::say(crawlInfo -> clientId);
            return false;
        }
}

bool CrawlerHelper::dealJson() {
    #if MORE_DETAILS
        cppUtil::say("爬取中");
    #endif

    if(!tempData.empty()) {
        if(pluginDealJson(tempData))
            return true;

        try {
            json = Json::parse(tempData);
    #ifdef DEVELOP
            cppUtil::say("保存此次爬取临时数据");
            dataStore::writeToJson(json,tempDataName,tempDataPath);
    #endif
        }catch (const exception&){
    #ifdef DEVELOP
            cppUtil::warn("爬取数据格式错误！");
            cppUtil::warn("数据如下：");
            cppUtil::warn(tempData);
    #endif
            const auto* handler = crawlInfo != nullptr && crawlInfo -> client != nullptr
                ? crawlInfo -> client -> handler()
                : nullptr;
            if (handler != nullptr && !handler -> validBrowse()) {
                cppUtil::warn({false, nullptr}, "请检查COOKIE是否正常，客户端ID: ");
                cppUtil::warn(crawlInfo -> clientId);
            }
            return false;
        }
    }
    auto task = crawlTask::nowTask();
    if (crawlInfo == nullptr || crawlInfo -> client == nullptr) {
        cppUtil::warn("Client not initialized, cannot deal json");
        return false;
    }
    auto* handler = crawlInfo -> client -> handler();
    if (handler == nullptr) {
        cppUtil::warn("Client handler not initialized, call /set or /login first");
        return false;
    }
    return handler -> dealJson(*this, json, task);
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
    if (crawlInfo == nullptr || crawlInfo -> client == nullptr) {
        cppUtil::warn("Client not initialized, cannot refresh subscribers");
        return false;
    }
    auto* handler = crawlInfo -> client -> handler();
    if (handler == nullptr) {
        cppUtil::warn("Client handler not initialized, call /set or /login first");
        return false;
    }
    return handler -> refreshSubscribers(*this, force);
}

bool CrawlerHelper::crawlNext() const {
    return _crawlNext || (crawlTask::nowTask() -> mode != crawlTask::WorkingMode::SUBSCRIBE);
}

void CrawlerHelper::addSubscriber(const dataStore::Data& _subscribers) {
    if (subscribers.is_object() && _subscribers.is_object()) {
        for (const auto& [key,value] : _subscribers.items()) {
            if (subscribers.contains(key) && subscribers[key].is_array() && value.is_array())
                subscribers[key].insert(subscribers[key].end(), value.begin(), value.end());
            else if (subscribers.contains(key) && subscribers[key].is_object() && value.is_object())
                subscribers[key].update(value);
            else
                subscribers[key] = value;
        }
    }else
        subscribers = _subscribers;
}

void CrawlerHelper::clearSubscriber() {
    subscribers.clear();
}

dataStore::Data CrawlerHelper::getSubscribers(const string& name) {
    if (name.empty())
        return subscribers;
    Json sub = subscribers;
    for (auto &up: _getListFromData(sub, false).items()) {
        if (_getSubscriberName(up.value()) == name)
            return up.value();
    }
    return dataStore::Data();
}

#define CLIENT_COOKIE (crawlInfo -> client -> handler() -> getCOOKIE().c_str())
webAPI::DbConfig postgresConfig;
webAPI::postgres dataBase(postgresConfig);
string browseManagerUrl;
[[deprecated]] string user_agent;

bool crawl(const std::shared_ptr<const std::atomic<bool>>& cancel,boost::asio::ip::tcp::socket& socket){
    const auto& session = getSession();
    sendSession(socket,session);
    const int max_count = config<int>(MAX_CRAWL_COUNT);
    int count = 0;
    try {
        const auto& handler = crawlInfo -> client -> handler();
        bool back = true;
        do{
            count++;

    #if SLEEP_CRAWL
            cppUtil::say("爬取等待...");
        #ifdef WIN32
            Sleep(config<int>(WAIT_TIME));
        #elifdef __linux__
            sleep(config<int>(WAIT_TIME));
        #endif
    #endif
            const auto& task = crawlTask::nowTask();
            back &= handler -> dealJson(webAPI::getController().perform(handler -> getWorker(task)),task);
            if (task)
                task -> workOnce();
            if (webAPI::enoughVideo()) {
                if (crawlTask::nextTask(false) != nullptr)
                    crawlTask::nextTask(true);
                else break;
            }
        }while(!cancel -> load() && back && count < max_count);

        back &= handler -> getWorker(crawlTask::nowTask()) == webAPI::nullWorker();
        return writeSession(session,webAPI::getVideoJson(),!back) && back;
    }catch(const std::exception& e) {
        cppUtil::warn("爬取失败，遇到错误！",e.what());
        writeSession(session,Json(),true);
        return false;
    }
}

string getURL(const crawlTask::Task* task){
    auto url = pluginGetURL();
    if(!url.empty())
        return url;
    if (crawlInfo == nullptr || crawlInfo -> client == nullptr) {
        cppUtil::warn("Client not initialized, cannot get url");
        return "";
    }
    auto* handler = crawlInfo -> client -> handler();
    if (handler == nullptr) {
        cppUtil::warn("Client handler not initialized, call /set or /login first");
        return "";
    }
    return handler -> getWorker(task).context -> cookie;
}

webAPI::BrowseWorker getWorker(const crawlTask::Task* task) {
    auto worker = pluginGetWorker();
    if (worker.valid())
        return worker;
    if (crawlInfo == nullptr || crawlInfo -> client == nullptr) {
        cppUtil::warn("Client not initialized, cannot get url");
        return webAPI::nullWorker();
    }
    auto* handler = crawlInfo -> client -> handler();
    if (handler == nullptr) {
        cppUtil::warn("Client handler not initialized, call /set or /login first");
        return webAPI::nullWorker();
    }
    return handler -> getWorker(task);
}

#define LOG_ENV(NAME) \
    string err = "未找到环境变量: "; \
    err += (NAME); \
    cppUtil::warn(err); \
    error |= true;

bool checkEnv(){
    bool error = false;
    if (getenv(POSTGRES_SCHEMA) == nullptr || getenv(POSTGRES_USER) == nullptr || getenv(POSTGRES_PASSWORD) == nullptr || getenv(POSTGRES_HOST) == nullptr || getenv(POSTGRES_PORT) == nullptr) {
        LOG_ENV(POSTGRES_SCHEMA "或" POSTGRES_USER "或" POSTGRES_PASSWORD "或" POSTGRES_HOST "或" POSTGRES_PORT);
    }else {
        postgresConfig = webAPI::DbConfig(getenv(POSTGRES_HOST),stoi(getenv(POSTGRES_PORT)),getenv(POSTGRES_SCHEMA),getenv(POSTGRES_USER),getenv(POSTGRES_PASSWORD));
        dataBase = webAPI::postgres(postgresConfig);
        cppUtil::say("数据库初始化");
        error |= !dataBase.init();
        if (error)
            cppUtil::warn("数据库初始化失败！");
        else cppUtil::say("数据库初始化完成");
    }
    /*if(user_agent.empty()){
        if (getenv(USERAGENT) == nullptr){
            LOG_ENV(USERAGENT);
        }else user_agent = getenv(USERAGENT);
    }
    user_agent = "";*/
    if (!getenv(BROWSE_MANAGER_URL)) {
        browseManagerUrl = "http://browser:3000"; // Docker compose 内部子网使用
    }else browseManagerUrl = getenv(BROWSE_MANAGER_URL);
    webAPI::BrowseController::controller = webAPI::BrowseController(browseManagerUrl);
    return error;
}

bool webAPI::socialAPI::checkVideo(const Video & video) const {
    webAPI::setVideo(&video);
    const auto* group = crawlTask::getGroup();
    const char* groupName = group == nullptr ? "" : group -> name;
    if (duplicateVideo(video,groupName,this -> support().c_str()))
        return true;
    if(roughCheckVideo() && finalCheckVideo()) {
        webAPI::keepVideo(video,groupName,this -> support().c_str());
        return true;
    }
    return false;
}
