#pragma once

#include <boost/asio.hpp>

struct CrawlInfo {
    const std::string target;
    const long long id;

    CrawlInfo(std::string target, long long id);
};

extern thread_local CrawlInfo const* crawlInfo;

void startWork();

void sendMessage(boost::asio::ip::tcp::socket& socket,std::string data = "");

std::string startCrawlForURL(const std::string& url);

void cacheData(const std::string& url,const std::string& data);

std::string getData(const std::string& url);