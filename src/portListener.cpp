#include "PortListener.h"
#include "PluginHandler.h"
#include "BilibiliInterface.h"
#include "Crawler.h"
#include "config.h"
#include <iostream>
#include "develop/flags.h"
#include "PortListener.h"
#include <boost/beast/http.hpp>
#include <boost/url.hpp>
#include <boost/beast/core.hpp>

#ifdef TEST
    #include "test/testCode.h"
#endif

#define SERVER_HEADER NAME_STR "/" VERSION_STR
namespace http = boost::beast::http;
namespace ip = boost::asio::ip;

#if NEED_PORT
thread_local CrawlInfo const* crawlInfo;
thread_local shared_ptr<const atomic<bool>> stop;

CrawlInfo::CrawlInfo(std::string url,std::string target, long long id)
    : url(std::move(url)), target(std::move(target)), id(id) {
}


int work(CrawlInfo info,shared_ptr<const atomic<bool>> cancel,ip::tcp::socket socket);
auto WorkFunction = &work;

void startWork() {
    readConfig();
    PluginHandler::loadAll();
    auto* tempHelper = new CurlHelper();
    tempHelper -> curlSetup(cookie,user_agent);
    tempHelper -> refreshSubscribers();
    delete tempHelper;
    say("Listening thread start");
    try {
        boost::asio::io_context io;
        ip::tcp::acceptor acceptor(io,ip::tcp::endpoint(ip::tcp::v4(),config<int>(PORT)));
        long long id = 1;
        const bool details = config<bool>(DETAILS);
    #ifdef TEST
        startTestThread();
        while (!testFinished){
    #else
        while(true) {
    #endif
            ip::tcp::socket socket(io);
            acceptor.accept(socket);
            boost::beast::flat_buffer buffer;
            http::request<http::string_body> request;
            try {
                http::read(socket, buffer, request);
            }catch (boost::system::system_error& e) {
                if (e.code() != boost::asio::error::eof) {
                    warn("HTTP read error: ", false);
                    warn(e.what());
                }
                socket.close();
                continue;
            }
            std::string url = request.target();
            if (!needCrawlURL(url)) {
                socket.close();
                continue;
            }
            boost::urls::url_view p = *boost::urls::parse_uri_reference(url);

            std::string category;
            if (p.has_query())
                for (auto const& param : p.params()) {
                    if (param.key == URL_PARAMS_CATEGORY)
                        category = param.value;
                }
            if (details) {
                say("Create thread for client from",false);
                cout << socket.remote_endpoint() << endl;
                say("Request URL: ",false);
                cout << url << endl;
                boost::asio::streambuf request_buffer;
                boost::system::error_code error;
                if (!error || error == boost::asio::error::eof) {
                    // EOF 可能意味着客户端发送完数据后断开连接
                    std::istream request_stream(&request_buffer);
                    std::string line;
                    say("Received network request headers:");
                    // 逐行打印请求头
                    while (std::getline(request_stream, line) && line != "\r") { // HTTP 行通常以 \r\n 结束
                        say(line.c_str());
                    }
                }
            }

            CrawlInfo info(url,category,id);
            auto cancel = make_shared<atomic<bool>>(false);
            auto working = std::async(std::launch::async,WorkFunction,std::move(info),cancel,std::move(socket));
            std::thread newThread([](future<int> worker,const shared_ptr<atomic<bool>> cancel,long long id,const long long& timeout) {
                setThreadId(id);
                say("Thread has created.",false);
                say("Id: ",false,BLUE);
                say(to_string(id).c_str(),true,BLUE);
                const auto& status = worker.wait_for(std::chrono::milliseconds(timeout));
                if (status == std::future_status::timeout) {
                    warn("Thread timeout");
                    cancel -> store(true);
                }
            },std::move(working),std::move(cancel),id,config<int>(TIMEOUT));
            id++;

            newThread.detach();
        }
    }catch (std::exception& e) {
        warn("Listening to port encountered an error:");
        throwError(e.what());
    }
}

bool sendMessage(ip::tcp::socket& socket,string data) {
    if (data.empty()) {
        Json json = bilibili::getVideoJson();
        data = json.empty() ? "{}" : to_string(json);
    }
    http::response<http::string_body> response;
    response.version(11);
    response.result(http::status::ok);
    response.set(http::field::server,SERVER_HEADER);
    response.set(http::field::content_type, "application/json; charset=utf-8");
    response.set(http::field::connection, "close");
    response.body() = data;
    response.prepare_payload();
    boost::system::error_code error;
    http::write(socket,response,error);
    bool back = true;
    if (error) {
        warn("Error to send response, error code: ",false);
        warn(error.message().c_str());
        back = false;
    }
    socket.shutdown(ip::tcp::socket::shutdown_both,error);
    socket.close();
    if (error) {
        warn("Cannot close socket");
        warn(error.message().c_str());
        back = false;
    }
    return back;
}
#endif