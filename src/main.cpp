#include <iostream>
#include <utility>
#include <chrono>
#include "Crawler.h"
#include "PluginHandler.h"
#include "pluginInterface.h"
#include "develop/flags.h"
#include "config.h"
#include "PortListener.h"

void setup(){
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

void clean(){
    curl_global_cleanup();
}

#if NEED_PORT
int work(const string& target,const map<const string,std::any>& config,const std::atomic<bool>& cancel){
    defaultConfigs = config;
#else
int main(int argc, char** argv){
    string target;
    readConfig();
    PluginHandler::loadAll();
#endif

#ifdef WIN32
    warn("这个程序是为Linux系统设计的，对于Windows系统难保可用性，不建议在Windows上使用！");
#endif

    if(!checkEnv()) {
        return 1;
    }

    crawlTask::GroupFilter(target);
    PluginHandler::forEachPlugin([](PluginHandler& plugin) -> PluginStatus {
        return plugin.registerGroups();
    });

    #if TEST_DLL
    auto task = crawlTask::nowTask();
    say("第一个注册的任务：",false);
    say(task -> keyword,true,GREEN);
    say("其工作状态：",false);
    say(crawlTask::getName(task -> mode),true,GREEN);
    cout << "当前处于测试插件状态，主程序已退出" << endl;
    return 0;
    #endif

    setup();
    if(crawl()) {
        cout << "运行成功，现在将退出程序！";
        clean();
        return 0;
    }
    cout << "运行失败，请检查具体原因！";
    clean();
    return 1;
}
#if NEED_PORT
int (*WorkFunction)(const string&,const map<const string,std::any>&,const std::atomic<bool>&) = &work;
int main(int argc, char** argv) {
    say("Litstening thread start");
    readConfig();
    PluginHandler::loadAll();
    try {
        boost::asio::io_context io;
        boost::asio::ip::tcp::acceptor acceptor(io,boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),config<int>(PORT)));
        while(true) {
            boost::asio::ip::tcp::socket socket(io);
            acceptor.accept(socket);
            say("Create thread for client from",false);
            cout << socket.remote_endpoint() << endl;
            boost::asio::streambuf request_buffer;
            boost::system::error_code ec;
            size_t bytes_transferred = boost::asio::read_until(socket, request_buffer, "\r\n\r\n", ec);
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

            std::thread newThread([](boost::asio::ip::tcp::socket socket,const map<const string,std::any>& config,const long long& timeout) {
                std::atomic<bool> cancel(false);
                auto working = std::async(std::launch::async,WorkFunction,"a",config,std::ref(cancel));
                say("Thread has created.");
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
            },std::move(socket),defaultConfigs,config<int>(TIMEOUT));

            newThread.detach();
        }
    }catch (std::exception& e) {
        warn("Listening to port encountered an error:");
        throwError(e.what());
    }
}
#endif