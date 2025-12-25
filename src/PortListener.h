#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/url/params_view.hpp>

struct CrawlInfo {
    const boost::urls::params_view params;
    const std::string url;
    const std::string target;
    const std::string cookie;
    const bool set_cookie_env;
    const long long id;

    CrawlInfo(boost::urls::params_view params,std::string url,std::string target,bool set_cookie_env,std::string newCookie, long long id);
};

extern thread_local CrawlInfo const* crawlInfo;
extern thread_local std::shared_ptr<const std::atomic<bool>> stop;

void startWork();

bool sendMessage(boost::asio::ip::tcp::socket& socket,std::string data = "");