#include <boost/asio.hpp>
#include "PortListener.h"
#include "PluginHandler.h"
#include "BilibiliInterface.h"
#include "Crawler.h"
#include "config.h"
#include <iostream>
#include "develop/flags.h"

#if NEED_PORT
int work(const string& target,const map<const string,std::any>& config,const std::atomic<bool>& cancel);
int (*WorkFunction)(const string&,const map<const string,std::any>&,const std::atomic<bool>&) = &work;

void startWork() {
    say("Litstening thread start");
    readConfig();
    PluginHandler::loadAll();
    auto* tempHelper = new CurlHelper();
    tempHelper -> curlSetup(cookie,user_agent);
    tempHelper -> refreshSubscribers();
    delete tempHelper;
    try {
        boost::asio::io_context io;
        boost::asio::ip::tcp::acceptor acceptor(io,boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),config<int>(PORT)));
        long long id = 0;
        const bool details = config<bool>(DETAILS);
        while(true) {
            boost::asio::ip::tcp::socket socket(io);
            acceptor.accept(socket);
            say("Create thread for client from",false);
            if (details) {
                cout << socket.remote_endpoint() << endl;
                boost::asio::streambuf request_buffer;
                boost::system::error_code ec;
                if (!ec || ec == boost::asio::error::eof) {
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

            std::thread newThread([](boost::asio::ip::tcp::socket socket,const map<const string,std::any>& config,const long long& timeout,const long long& id) {
                std::atomic<bool> cancel(false);
                auto working = std::async(std::launch::async,WorkFunction,"a",config,std::ref(cancel));
                say("Thread has created.",false);
                say("Id: ",false,BLUE);
                say(to_string(id).c_str(),true,BLUE);
                const auto& status = working.wait_for(std::chrono::milliseconds(timeout));
                if (status == std::future_status::timeout) {
                    warn("Thread timeout");
                    cancel = true;
                }

                try {
                    working.get();
                    socket.close();
                }catch (std::exception& e) {
                    socket.close();
                    warn("Cannot close the thread ! Details below: ");
                    throwError(e.what());
                }
            },std::move(socket),defaultConfigs,config<int>(TIMEOUT),id);
            id++;

            newThread.detach();
        }
    }catch (std::exception& e) {
        warn("Listening to port encountered an error:");
        throwError(e.what());
    }
}

void sendMessage() {
    const auto& videos = bilibili::getVideos();
}
#endif