#pragma once
#include "pluginInterface.h"
#include <boost/asio/ip/tcp.hpp>

#include "utils/schedules.h"
#include "webAPIs/frontend/PlatformFormater.h"

#define REQUIRED
#define OPTIONAL

#ifdef WIN32
   #define FUNCTION_CALLER __cdecl
#elifdef __linux__
   #define FUNCTION_CALLER
#endif

extern "C" {
/**
 * Called at program starting
 * */
REQUIRED PluginStatus load();
REQUIRED typedef PluginStatus (FUNCTION_CALLER *LOAD)();

/**
 * Register all video group and tasks you want to crawl for.
 * */
OPTIONAL void registerGroups();
OPTIONAL typedef void (FUNCTION_CALLER *REGISTER)();

/**
 * Rough judge whether program need to crawl more data , keep it in website or skip this video
 * */
OPTIONAL VideoStatus roughJudge(const webAPI::Video& video);
typedef VideoStatus (FUNCTION_CALLER *ROUGH_JUDGE)(const webAPI::Video& video);

/**
 * Judge keep or throw this video away
 * */
OPTIONAL VideoStatus judge(const webAPI::Video& video);
typedef VideoStatus (FUNCTION_CALLER *JUDGE)(const webAPI::Video& video);

/**
 * Now deprecated
 * Get this specific url for now task
 * @return "" means failed
 * */
OPTIONAL [[deprecated]] const char* getURL();
typedef const char* (FUNCTION_CALLER *GETURL)();

namespace webAPI {
 class Video;
 class BrowseWorker;
}
OPTIONAL webAPI::BrowseWorker getWorker();
typedef webAPI::BrowseWorker (FUNCTION_CALLER *GETWORKER)();

/**
 * Deal this specific json for now crawling
 * @return true means crawl succeed
 * */
OPTIONAL bool dealJson(const char* data);
typedef bool (FUNCTION_CALLER *DEAL_JSON)(const char* data);

/**
 * Allow plugin to handle network request from client
 * @return success or not, examples see exit.h
 */
OPTIONAL int dealRequest(boost::asio::ip::tcp::socket& socket,const Json& data);
typedef int (FUNCTION_CALLER *DEAL_REQUEST)(boost::asio::ip::tcp::socket& socket,const Json& data);

/**
 * Rely on client feedback change video feeds
 * @param feedback Client feedback
 */
OPTIONAL void feedback(const webAPI::formater::FeedBack& feedback);
typedef void (FUNCTION_CALLER *FEED_BACK)(const webAPI::formater::FeedBack& feedback);

/**
 * Register the schedule crawl task
 * @return The tasks those will be triggered by main thread on time, which use the empty time and collect videos
 */
OPTIONAL vector<webAPI::schedules::ScheduleTask> scheduleCrawl();
typedef vector<webAPI::schedules::ScheduleTask> (FUNCTION_CALLER *ScheduleCrawl)();
}

namespace webAPI::formater {
 class PlatformFormater;

 /**
  * Plugin register formater, for frontend asked format platform's user prefers
  * @param adder may throw error (webAPI::formater::SameException), you may need try-catch
  */
 extern "C" OPTIONAL void registerFormater(std::function<bool(const PlatformFormater&)> adder);
 typedef void (FUNCTION_CALLER *REGISTER_FORMATER)(std::function<bool(const PlatformFormater&)> adder);
}

#define URL_PARAMS_CATEGORY "category"
#define BODY_PARAMS_PLATFORM "platform"
#define BODY_PARAMS_NAME "name"
#define BODY_PARAMS_VIDEO "video"
#define BODY_PARAMS_SCORE "score"
//#define URL_PARAMS_USERNAME "username"
//#define URL_PARAMS_PASSWORD "password"
#define URL_PARAMS_CLIENT_ID "id"
#define URL_PARAMS_TEST "test"
#define URL_PARAMS_SESSION "session"
#define BODY_PARAMS_ENCRYPT_KEY "key"
#define BODY_PARAMS_ADMIN "admin"