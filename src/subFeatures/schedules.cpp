#include "utils/schedules.h"

#include <random>
#include <thread>
#include <atomic>

#ifdef TEST
    #include <condition_variable>
    #include <mutex>
#endif

#include "PortListener.h"
#include "../Crawler.h"
#include "../PluginHandler.h"
#include "utils/Util.h"
#include "webAPIs/socialAPI.h"

namespace webAPI::schedules {
    thread_local vector<ScheduleTask> scheduleTasks;

    namespace {
        std::jthread scheduleThread;
        std::atomic<bool> scheduleRunning{false};
    }

#ifdef TEST
    namespace {
        std::condition_variable clientCrawlCondition;
        std::mutex clientCrawlMutex;
        unsigned long long clientCrawlGeneration = 0;
    }

    bool waitForClientCrawl(const int timeoutMs) {
        std::unique_lock lock(clientCrawlMutex);
        const auto generation = clientCrawlGeneration;
        return clientCrawlCondition.wait_for(
            lock,
            std::chrono::milliseconds(timeoutMs),
            [generation] {
                return clientCrawlGeneration != generation;
            }
        );
    }
#endif

    bool registerScheduleTask(const ScheduleTask& task) {
        scheduleTasks.push_back(task);
        return true;
    }

    void startScheduleThread() {
        scheduleRunning = true;
        scheduleThread = std::jthread([](const std::stop_token& st) {
            try {
                cppUtil::say("Schedule thread start");
                PluginHandler::registerAllScheduleTasks();
                time_t time;
                std::default_random_engine random;
                auto range = std::uniform_real_distribution<float>(0.8, 1.2);
                while (!st.stop_requested()) {
                    while (!st.stop_requested() && !finishClient()) {
                        const auto& client = nextClient();
                        if (client == nullptr)
                            continue;
                    #ifdef WIN32
                        Sleep(config<int>(SCHEDULE_CRAWL_INTERNAL) * 60000 * range(random));
                    #elifdef __linux__
                        sleep(config<int>(SCHEDULE_CRAWL_INTERNAL) * 60 * range(random));
                    #endif
                        client -> nextPreCrawl();
                        crawlInfo = new CrawlInfo(client -> getID(),"",{},"","",0);
                        cppUtil::say("定时爬取，工作客户端ID: ",client -> getID());
                        for (const ScheduleTask& task : scheduleTasks) {
                            ::time(&time);
                            if (!crawlAndStore(task(std::localtime(&time),client),client))
                                cppUtil::warn("定时爬取",client -> handler() -> support(),"失败，客户端ID: ",client -> getID());
                        }
                        client -> syncPreCrawlDataBase();
                        cppUtil::say("此次定时爬取完成");
                #ifdef TEST
                        {
                            std::lock_guard lock(clientCrawlMutex);
                            clientCrawlGeneration++;
                        }
                        clientCrawlCondition.notify_all();
                #endif
                        delete crawlInfo;
                        crawlInfo = nullptr;
                    }
                    if (st.stop_requested())
                        break;
                    say("一轮自动工作已完成，自动同步客户端");
                    Client::syncWithDatabase();
                }
            } catch (const std::exception& e) {
                cppUtil::warn("Schedule thread error:", e.what());
            } catch (...) {
                cppUtil::warn("Schedule thread unknown error");
            }
            scheduleRunning = false;
        });
    }

    void stopScheduleThread() {
        if (scheduleThread.joinable())
            scheduleThread.request_stop();
    }

    bool isScheduleThreadRunning() {
        return scheduleRunning.load();
    }

    static int index = 0;

    Client *nextClient() {
        for (const auto& clients = Client::allClients();index < clients.size(); index++) {
            const auto client = clients[index];
            if (client -> handler() == nullptr)
                continue;
            index++;
            return client;
        }
        return nullptr;
    }

    bool finishClient() {
        const auto& clients = Client::allClients();
        const bool& back = index >= clients.size();
        index = 0;
        if (clients.empty() || nextClient() == nullptr) {
            cppUtil::warn("当前没有客户端可爬取，定时任务线程休眠",config<int>(SCHEDULE_CRAWL_INTERNAL),"分钟");
            #ifdef WIN32
                Sleep(config<int>(SCHEDULE_CRAWL_INTERNAL) * 60000);
            #elifdef __linux__
                sleep(config<int>(SCHEDULE_CRAWL_INTERNAL) * 60);
            #endif
        }
        index = 0;
        return back;
    }
}
