#include "PortListener.h"
#include "PluginHandler.h"
#include "utils/BilibiliInterface.h"
#include "Crawler.h"
#include "utils/config.h"
#include <iostream>
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
        cppUtil::say(client == nullptr ? "空客户端ID" : "有效客户端ID");
        if (this -> body.empty()){
            if (body.empty())
                cppUtil::say("空body参数");
            else if (client != nullptr){
                cppUtil::say({false, nullptr}, "前端传输body参数秘钥错误，实际密钥：");
                cppUtil::say(client -> ESAKey(config<string>(ADMIN_CLIENT_KEY)));
                cppUtil::say({false, nullptr}, "而前端传输的body是：");
                cppUtil::say(body);
            }
        }else {
            cppUtil::say("本次登录body参数：");
            cppUtil::say(this -> body);
        }
    }
}

bool CrawlInfo::checkClient() const noexcept {
    return client != nullptr && client -> check();
}

int work(CrawlInfo info,shared_ptr<const atomic<bool>> cancel,ip::tcp::socket socket);
auto WorkFunction = &work;

int startWork() {
    if (!makeConfigDir())
        cppUtil::throwError("创建配置文件夹失败，退出程序");
    readConfig();
    if(checkEnv()) {
        return 1;
    }
    webAPI::registerBilibili();
    webAPI::Client::initAndDataBase();
    PluginHandler::loadAll();
    webAPI::schedules::startScheduleThread();
    cppUtil::say("Listening thread start");
    try {
        boost::asio::io_context io;
        ip::tcp::acceptor acceptor(io);
        acceptor.open(ip::tcp::v6());
        acceptor.set_option(boost::asio::socket_base::reuse_address(true));
        acceptor.set_option(boost::asio::ip::v6_only(false));
        acceptor.bind(ip::tcp::endpoint(ip::tcp::v6(), config<int>(PORT)));
        acceptor.listen();
        long long id = 1;
        const bool details = config<bool>(DETAILS);
    #ifdef TEST
        startTestThread();
        while (!testFinished){
    #else
        while(webAPI::schedules::isScheduleThreadRunning()) {
    #endif
            ip::tcp::socket socket(io);
            acceptor.accept(socket);
            boost::beast::flat_buffer buffer;
            http::request<http::string_body> request;
            try {
                http::read(socket, buffer, request);
            }catch (boost::system::system_error& e) {
                if (e.code() != boost::asio::error::eof) {
                    cppUtil::warn({false, nullptr}, "HTTP read error: ");
                    cppUtil::warn(e.what());
                }
                socket.close();
                continue;
            }
            const auto& url = request.target();
            auto&& u = boost::urls::parse_uri_reference(url);
            boost::urls::url_view p;
            if (u.has_value())
                p = *u;

            std::string category,clientId;
            if (p.has_query())
                for (auto const& param : p.params()) {
                    if (param.key == URL_PARAMS_CLIENT_ID)
                        clientId = param.value;
                    else if (param.key == URL_PARAMS_CATEGORY)
                        category = param.value;
                }
            if (details) {
                cppUtil::say({false, nullptr}, "Create thread for client from ");
                cout << socket.remote_endpoint() << endl;
                cppUtil::say({false, nullptr}, "Client Id: ");
                cppUtil::say(clientId.empty() ? "Unknown" : clientId);
                cppUtil::say({false, nullptr}, "Request URL: ");
                cppUtil::say(url);
            }

            CrawlInfo info(clientId,request.body(),p.has_query() ? p.params() : boost::urls::params_view(),url,category,id);
            auto cancel = make_shared<atomic<bool>>(false);
            auto working = std::async(std::launch::async,WorkFunction,std::move(info),cancel,std::move(socket));
            std::thread newThread([](future<int> worker,const shared_ptr<atomic<bool>> cancel,long long id,const long long& timeout) {
                setThreadId(id);
                cppUtil::say({false, nullptr}, "Thread has created.");
                cppUtil::say({false, BLUE}, "Id: ");
                cppUtil::say({true, BLUE}, id);
                const auto& status = worker.wait_for(std::chrono::milliseconds(timeout));
                if (status == std::future_status::timeout) {
                    cppUtil::warn("Thread timeout");
                    cancel -> store(true);
                }
                cout << endl;
            },std::move(working),std::move(cancel),id,config<int>(TIMEOUT));
            id++;

            newThread.detach();
        }
    }catch (std::exception& e) {
        cppUtil::warn("Listening to port encountered an error:");
        webAPI::schedules::stopScheduleThread();
        cppUtil::throwError(e.what());
        return 1;
    }
    webAPI::schedules::stopScheduleThread();
    return 0;
}

bool sendMessage(ip::tcp::socket& socket, string data, bool failed, bool releaseOutput) {
    if (data.empty()) {
        cppUtil::warn("空返回数据！");
        return false;
    }
    http::response<http::string_body> response;
    response.version(11);
    response.result(failed ? http::status::internal_server_error : http::status::ok);
    response.set(http::field::server,SERVER_HEADER);
    response.set(http::field::content_type, "application/json; charset=utf-8");
    response.set(http::field::connection, "close");
    response.body() = crawlInfo -> client == nullptr ? data : crawlInfo -> client -> encrypt(data);
    if (releaseOutput && config<bool>(DETAILS))
        cppUtil::say("本次工作结果（未加密）：",data);
    response.prepare_payload();
    boost::system::error_code error;
    http::write(socket,response,error);
    bool back = true;
    if (error) {
        cppUtil::warn({false, nullptr}, "Error to send response, error code: ",error.message());
        back = false;
    }
    socket.shutdown(ip::tcp::socket::shutdown_both,error);
    socket.close();
    if (error) {
        cppUtil::warn("Cannot close socket",error.message());
        back = false;
    }
    return back;
}
