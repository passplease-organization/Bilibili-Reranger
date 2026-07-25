#pragma once
#include "../APIStatus.h"
#include "../pluginInterface.h"

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

    API bool registerScheduleTask(const ScheduleTask& task);


    /**
     * Designed prepared for muti-thread pre-crawl, which is single thread for now
     * @return next pre-crawl client
     */
    API Client* nextClient();

    API bool finishClient();

    inline API void throwRegisterError() {
        throw std::runtime_error("Register timer task fails !");
    }

    void startScheduleThread();
    void stopScheduleThread();
    [[nodiscard]] bool isScheduleThreadRunning();

#ifdef TEST
    API bool waitForClientCrawl(const int timeoutMs);
#endif
}
