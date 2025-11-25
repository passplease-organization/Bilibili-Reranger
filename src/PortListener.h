#pragma once

#include <boost/asio/ip/tcp.hpp>

struct CrawlInfo {
    const std::string url;
    const std::string target;
    const long long id;

    CrawlInfo(std::string url,std::string target, long long id);
};

extern thread_local CrawlInfo const* crawlInfo;
extern thread_local std::shared_ptr<const std::atomic<bool>> stop;

void startWork();

bool sendMessage(boost::asio::ip::tcp::socket& socket,std::string data = "");