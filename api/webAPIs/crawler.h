#pragma once

class [[deprecated]] CrawlerHelper : public webAPI::CurlHelper {
    friend string getURL(const crawlTask::Task* task);
    friend webAPI::BrowseWorker getWorker(const crawlTask::Task* task);

public:
    CrawlerHelper();
    CrawlerHelper(dataStore::Data subscribers);

    ~CrawlerHelper() override = default;

    void curlSetup(const string &cookie,const string& useragent);

    void curlSetup() override;

    bool connect(bool deal) override;
    bool connect() override{
        return connect(true);
    }

    bool dealJson() override;

    void addSubscriber(const dataStore::Data& _subscribers);

    void clearSubscriber();

    dataStore::Data getSubscribers(const string& name = "");

    dataStore::Data subscribers;

private:
    bool _crawlNext = true;

    static bool nextPage();

    static unsigned int getPages(const string& url);

    void nextPage(unsigned int nowPage);

    void nextMustCrawl();

    void setURL(const string &url) override {}

public:
    [[nodiscard]] bool finishCrawl() const;

    void nextSearch(const string& url);

    bool refreshSubscribers(bool force = false);

    [[nodiscard]] bool crawlNext() const;

    void clearData() {
        clear();
    }

    void clearNextURL() {
        clearURL();
    }

    void markMustCrawl() {
        nextMustCrawl();
    }

    void advancePage(unsigned int nowPage) {
        nextPage(nowPage);
    }

    static unsigned int parsePages(const string& url) {
        return getPages(url);
    }

    [[nodiscard]] const string& rawData() const {
        return tempData;
    }
};
