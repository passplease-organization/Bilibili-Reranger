#include "PortListener.h"
#include "PluginHandler.h"
#include "BilibiliInterface.h"
#include "Crawler.h"
#include "config.h"
#include <iostream>
#include "develop/flags.h"
#include <boost/beast/http.hpp>
#include <boost/url.hpp>
#include <boost/beast/core.hpp>
#include <utility>

#include "platforms/bilibiliHandler.h"
#include "subFeatures/requestHelper.h"
#include "webAPIs/postgres.h"

#ifdef TEST
    #include "test/testCode.h"
#endif

#define SERVER_HEADER NAME_STR "/" VERSION_STR
namespace http = boost::beast::http;
namespace ip = boost::asio::ip;

#if NEED_PORT
thread_local CrawlInfo const* crawlInfo;
thread_local shared_ptr<const atomic<bool>> stop;

CrawlInfo::CrawlInfo(std::string clientId,const string& body,boost::urls::params_view params,std::string url,std::string target, long long id)
:client(webAPI::client(std::move(clientId))),
clientId(std::move(clientId)),
params(params),
url(std::move(url)),
target(std::move(target)),
id(id) {
    try {
        if (checkClient())
            this -> body = Json::parse(client -> decrypt(body));
        else this -> body = Json::parse(body);
    }catch (...) {
        this -> body = Json();
    }
    if (config<bool>(DETAILS)) {
        say(client == nullptr ? "空客户端ID" : "有效客户端ID");
        if (this -> body.empty()){
            if (body.empty())
                say("空body参数");
            else if (client != nullptr){
                say("前端传输body参数秘钥错误，实际密钥：",false);
                say(client -> ESAKey(config<string>(ADMIN_CLIENT_KEY)).c_str());
                say("而前端传输的body是：",false);
                say(body.c_str());
            }
        }else {
            say("本次登录body参数：");
            say(this -> body.dump().c_str());
        }
    }
}

int work(CrawlInfo info,shared_ptr<const atomic<bool>> cancel,ip::tcp::socket socket);
auto WorkFunction = &work;

int startWork() {
    readConfig();
    if(checkEnv()) {
        return 1;
    }
    webAPI::registerBilibili();
    webAPI::Client::initAndDataBase();
    PluginHandler::loadAll();
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

            std::string category,clientId;
            if (p.has_query())
                for (auto const& param : p.params()) {
                    if (param.key == URL_PARAMS_CLIENT_ID)
                        clientId = param.value;
                    else if (param.key == URL_PARAMS_CATEGORY)
                        category = param.value;
                }
            if (details) {
                say("Create thread for client from ",false);
                cout << socket.remote_endpoint() << endl;
                say("Client Id: ",false);
                say(clientId.empty() ? "Unknown" : clientId.c_str());
                say("Request URL: ",false);
                say(url.c_str());
            }

            CrawlInfo info(clientId,request.body(),p.params(),url,category,id);
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
                cout << endl;
            },std::move(working),std::move(cancel),id,config<int>(TIMEOUT));
            id++;

            newThread.detach();
        }
    }catch (std::exception& e) {
        warn("Listening to port encountered an error:");
        throwError(e.what());
        return 1;
    }
    return 0;
}

bool sendMessage(ip::tcp::socket& socket, string data, bool failed, bool releaseOutput) {
    if (data.empty()) {
        Json json = webAPI::getVideoJson();
        data = json.empty() ? "{}" : to_string(json);
    }
    http::response<http::string_body> response;
    response.version(11);
    response.result(failed ? http::status::internal_server_error : http::status::ok);
    response.set(http::field::server,SERVER_HEADER);
    response.set(http::field::content_type, "application/json; charset=utf-8");
    response.set(http::field::connection, "close");
    response.body() = crawlInfo -> client == nullptr ? data : crawlInfo -> client -> encrypt(data);
    if (releaseOutput && config<bool>(DETAILS)) {
        say("本次工作结果（未加密）：");
        say(data.c_str());
    }
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
