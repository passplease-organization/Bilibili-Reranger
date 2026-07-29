#include "PortListener.h"
#include "PluginHandler.h"
#include "Crawler.h"
#include "utils/config.h"
#include <iostream>
#include <boost/beast/http.hpp>
#include <boost/url.hpp>
#include <boost/beast/core.hpp>
#include <utility>

#include "platforms/bilibiliHandler.h"
#include "subFeatures/requestHelper.h"
#include "utils/Event.h"
#include "webAPIs/postgres.h"

#ifdef TEST
    #include "test/testCode.h"
#endif

namespace http = boost::beast::http;
namespace ip = boost::asio::ip;

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
    registerExitListener();
    Event::setExporter(PluginHandler::exportEvent);
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
                blockSignal();
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

void stopWork() {
    cppUtil::say(GREEN,"退出信号监听器已注册");
    const int signal = waitForExitSignal();
    cppUtil::say(GREEN,"收到退出信号，即将退出程序，信号",signal);
    webAPI::schedules::stopScheduleThread();
    for (int i = 0; i < 100; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (!webAPI::schedules::isScheduleThreadRunning())
            break;
    }
    if (webAPI::schedules::isScheduleThreadRunning()) {
        cppUtil::warn("定时线程仍未退出，将自动进行后续插件卸载，卸载插件后将会强制退出");
    }
    PluginHandler::unloadAll();
#ifdef TEST
    markExitCleanupFinished();
#endif
    if (webAPI::schedules::isScheduleThreadRunning()) {
        cppUtil::warn("定时线程仍未结束，将强制退出");
        raise(SIGKILL);
    }
}
