#pragma once
#include "../APIStatus.h"
#include "../pluginInterface.h"
#include <stop_token>

#ifdef WIN32
   #define FUNCTION_CALLER __cdecl
#elifdef __linux__
   #define FUNCTION_CALLER
#endif

namespace webAPI {
    class Client;
}

namespace webAPI::schedules {
    typedef crawlTask::Group (FUNCTION_CALLER *ScheduleTask)(std::tm* const& now,const Client* client);

    thread_local inline API bool preCrawl = false;
    inline API bool isPreCrawlThread() {
        return preCrawl;
    }

    bool registerScheduleTask(const ScheduleTask& task);
    bool registerTempScheduleTask(crawlTask::Task const& task);
    #define MAX_TEMP_TASK_WORK_COUNT 10

    /**
     * Designed prepared for muti-thread pre-crawl, which is single thread for now
     * @return next pre-crawl client
     */
    Client* nextClient();

    bool finishClient(const std::stop_token* stopToken = nullptr);

    inline API void throwRegisterError() {
        throw std::runtime_error("Register timer task fails !");
    }

    void startScheduleThread();
    void stopScheduleThread();
    [[nodiscard]] bool isScheduleThreadRunning();

#ifdef TEST
    bool waitForClientCrawl(const int timeoutMs);
#endif
}
