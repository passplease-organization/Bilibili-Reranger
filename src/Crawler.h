#pragma once
#include <atomic>
#include <curl/curl.h>
#include "pluginInterface.h"

using namespace std;

extern bool roughCheckVideo();
extern bool finalCheckVideo();
extern bool pluginDealJson(string&);
extern string pluginGetURL();

class CurlHelper{
friend string getURL(const crawlTask::Task* task);

public:
    CurlHelper();

    ~CurlHelper();

    void clear();

    void curlSetup(const char* cookie,const char* useragent);

    bool connect(bool deal = true);

    CURL* getCurl() const;

    bool dealJson();

    void dealJson(Json& _json);

private:
    static dataStore::Data subscribers;

    bool _crawlNext = true;

    CURL *curl;

    Json json = Json();

    string tempData;

    static size_t saveData(char *data, size_t size, size_t member, void *userdata);

    string url;

    [[nodiscard]] const string& nextURL() const;

    void clearURL();

    static bool nextPage();

    static unsigned int getPages(const string& url);

    void nextPage(unsigned int nowPage);

    void nextMustCrawl();

public:
    [[nodiscard]] bool finishCrawl() const;

    void nextSearch(const string& url);

    void refreshSubscribers(const bool force = false);

    [[nodiscard]] bool crawlNext() const;
};

const string search = "https://api.bilibili.com/x/web-interface/search/all/v2?nextTask=选择公理&search_type=video&page_size=10";
const string commit = "https://api.bilibili.com/x/v2/reply/main?oid=BV18a411W7ge&type=1";
const string subscribe = "https://api.bilibili.com/x/relation/followings?vmid=3493105986702255";
const string liked = "https://api.bilibili.com/x/space/like/video?vmid=3493105986702255";

extern const char* cookie;
extern const char* user_agent;

bool crawl(const std::atomic<bool>& cancel);

string getURL(const crawlTask::Task* task = crawlTask::nowTask());

bool checkEnv();