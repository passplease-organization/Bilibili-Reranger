#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "PortListener.h"
#include "utils/BilibiliInterface.h"
#include <boost/beast/http.hpp>
#include <boost/url.hpp>
#include <boost/beast/core.hpp>
#include "webAPIs/socialAPI.h"
#include "utils/config.h"

thread_local CrawlInfo const* crawlInfo;

namespace http = boost::beast::http;
namespace ip = boost::asio::ip;

#define SERVER_HEADER NAME_STR "/" VERSION_STR
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

sigset_t* mask;

namespace {
#ifdef TEST
    std::condition_variable exitCleanupCondition;
    std::mutex exitCleanupMutex;
    std::atomic<bool> exitCleanupFinished = false;
#endif
}

void blockSignal() {
    pthread_sigmask(SIG_BLOCK,mask,nullptr);
}

int waitForExitSignal() {
    int signal;
    sigwait(mask,&signal);
    return signal;
}

void registerExitListener() {
    mask = new ::__sigset_t();
    sigemptyset(mask);
    sigaddset(mask,SIGINT);
    sigaddset(mask,SIGTERM);
    sigaddset(mask,SIGQUIT);
    blockSignal();
    std::thread listener(stopWork);
    listener.detach();
}

#ifdef TEST
bool waitForExitCleanup(const int timeoutMs) {
    std::unique_lock lock(exitCleanupMutex);
    return exitCleanupCondition.wait_for(
        lock,
        std::chrono::milliseconds(timeoutMs),
        [] { return exitCleanupFinished.load(); }
    );
}

API void markExitCleanupFinished() {
    exitCleanupFinished = true;
    exitCleanupCondition.notify_all();
}
#endif
