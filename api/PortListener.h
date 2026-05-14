#pragma once

#include "develop/flags.h"
#include "utils/Util.h"

#include <atomic>
#include <memory>
#include <string>
#include <boost/asio/ip/tcp.hpp>
#include <boost/url/params_view.hpp>

namespace webAPI {
    class Client;
}

struct CrawlInfo {
    const boost::urls::params_view params;
    Json body;
    const std::string url;
    const std::string target;
    const long long id;
    webAPI::Client* client;
    std::string clientId;

    CrawlInfo(std::string clientId,const std::string& body,boost::urls::params_view params,std::string url,std::string target, long long id);

    [[nodiscard]] bool checkClient() const noexcept;

    #define BODY_CONTAIN(key) crawlInfo -> body.contains(key)
    #define INFO_BODY(key) crawlInfo -> body[key]
};

extern thread_local CrawlInfo const* crawlInfo;
extern thread_local std::shared_ptr<const std::atomic<bool>> stop;

int startWork();


API bool sendMessage(boost::asio::ip::tcp::socket& socket, std::string data = "", bool failed = false, bool releaseOutput = true);
