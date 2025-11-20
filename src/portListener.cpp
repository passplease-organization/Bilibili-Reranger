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

#define SERVER_HEADER NAME_STR "/" VERSION_STR
namespace http = boost::beast::http;
namespace ip = boost::asio::ip;

#if NEED_PORT
int work(const string& target, const map<const string,std::any>& config, const std::atomic<bool>& cancel, ip::tcp::socket& socket);
int (*WorkFunction)(const string&,const map<const string,std::any>&,const std::atomic<bool>&,ip::tcp::socket&) = &work;

void startWork() {
    readConfig();
    PluginHandler::loadAll();
    auto* tempHelper = new CurlHelper();
    tempHelper -> curlSetup(cookie,user_agent);
    tempHelper -> refreshSubscribers();
    delete tempHelper;
    say("Litstening thread start");
    try {
        boost::asio::io_context io;
        ip::tcp::acceptor acceptor(io,ip::tcp::endpoint(ip::tcp::v4(),config<int>(PORT)));
        long long id = 0;
        const bool details = config<bool>(DETAILS);
        while(true) {
            ip::tcp::socket socket(io);
            acceptor.accept(socket);
            boost::system::error_code error;
            boost::beast::flat_buffer buffer;
            http::request<http::string_body> request;
            http::read(socket, buffer, request);
            boost::urls::url_view p = *boost::urls::parse_uri_reference(request.target());

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
                cout << request.target() << endl;
                boost::asio::streambuf request_buffer;
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

            std::thread newThread([](ip::tcp::socket socket,const map<const string,std::any>& config,const long long& timeout,const long long& id,const std::string& category) {
                std::atomic<bool> cancel(false);
                auto working = std::async(std::launch::async,WorkFunction,category,config,std::ref(cancel),std::ref(socket));
                say("Thread has created.",false);
                say("Id: ",false,BLUE);
                say(to_string(id).c_str(),true,BLUE);
                const auto& status = working.wait_for(std::chrono::milliseconds(timeout));
                if (status == std::future_status::timeout) {
                    warn("Thread timeout");
                    cancel = true;
                }

                // try {
                //     working.get();
                //     socket.close();
                // }catch (std::exception& e) {
                //     socket.close();
                //     warn("Cannot close the thread ! Details below: ");
                //     throwError(e.what());
                // }
            },std::move(socket),defaultConfigs,config<int>(TIMEOUT),id,category);
            id++;

            newThread.detach();
        }
    }catch (std::exception& e) {
        warn("Listening to port encountered an error:");
        throwError(e.what());
    }
}

void sendMessage(ip::tcp::socket& socket) {
    Json json = "{}";
    for(const auto& group : bilibili::getVideos())
        for(int i = 0;i < group.second.size();i++) {
            group.second[i].write_necessary(json[group.first][i]);
        }
    http::response<http::string_body> response;
    response.version(11);
    response.result(http::status::ok);
    response.set(http::field::server,SERVER_HEADER);
    response.set(http::field::content_type, "application/json; charset=utf-8");
    response.set(http::field::connection, "close");
    response.body() = json;
    response.prepare_payload();
    boost::system::error_code error;
    http::write(socket,response,error);
    if (error) {
        warn("Error to send response, error code: ",false);
        warn(error.message().c_str());
    }
    socket.shutdown(ip::tcp::socket::shutdown_both,error);
    socket.close();
}
#endif