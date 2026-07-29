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

namespace Event {
 class Event;
}
extern "C" {
/**
 * Called at program starting
 * */
REQUIRED PluginStatus load();
REQUIRED typedef PluginStatus (FUNCTION_CALLER *LOAD)();

/**
 * Exit the plugin, not promised be called everytime, such as stopping container directly
 */
OPTIONAL void unload();
OPTIONAL typedef void (FUNCTION_CALLER *UNLOAD)();

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
 * Judge keep or throw this video away, called in pre-crawl
 * @param video the video to be judged
 * @param score how values are of videos, must have value if judged to be kept, max is 100, min is 0, normal should be 50
 * @return if we should keep this video
 */
#define MAX_VIDEO_SCORE 100
#define NORMAL_VIDEO_SCORE 50
#define MIN_VIDEO_SCORE 0
OPTIONAL VideoStatus judge(const webAPI::Video& video,unsigned short& score);
typedef VideoStatus (FUNCTION_CALLER *JUDGE)(const webAPI::Video& video,unsigned short& score);

/**
 * Same as above, but called when client asked, so need to be fast
 */
OPTIONAL VideoStatus judgeAndRecommend(const webAPI::Video& video,unsigned short& score);

/**
 * Tag each video, which will be used to search it from database
 * @param video the video will be stored into database
 * @param tagger tag function
 */
OPTIONAL void tagVideo(const webAPI::Video& video,std::function<bool(const char*)> tagger);
typedef void (FUNCTION_CALLER *TAG_VIDEO)(const webAPI::Video& video,std::function<bool(const char*)> tagger);

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
OPTIONAL webAPI::BrowseWorker getWorker(crawlTask::Task* const& task);
typedef webAPI::BrowseWorker (FUNCTION_CALLER *GETWORKER)(crawlTask::Task* const& task);

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

/**
 * Deal data, might got by task working mode PLUGIN, or comes from normal crawl including all origins
 * @param data The json data
 * @param task Now working task, might be registered by other plugin and also can be temp task
 * @param tempTasks This turn your temp tasks want to be registered
 * @param workCount Temp task work count, can't bigger than MAX_TEMP_TASK_WORK_COUNT
 * @return Ignored when task working mode is not PLUGIN, true means dealt, default by false, but won't skip any plugin even received true
 */
OPTIONAL bool pluginDealJson(const Json& data,crawlTask::Task* const& task,vector<crawlTask::Task>& tempTasks,const unsigned short& workCount);
typedef bool (FUNCTION_CALLER *DEAL_JSON)(const Json& data,crawlTask::Task* const& task,vector<crawlTask::Task>& tempTasks,const unsigned short& workCount);

OPTIONAL void eventListener(Event::Event* const& event);
typedef void (FUNCTION_CALLER *EVENT_LISTENER)(Event::Event* const& event);
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