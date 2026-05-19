#pragma once
#include "pluginInterface.h"
#include <boost/asio/ip/tcp.hpp>

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
OPTIONAL VideoStatus roughJudge();
typedef VideoStatus (FUNCTION_CALLER *ROUGH_JUDGE)();

/**
 * Judge keep or throw this video away
 * */
OPTIONAL VideoStatus judge();
typedef VideoStatus (FUNCTION_CALLER *JUDGE)();

/**
 * Now deprecated
 * Get this specific url for now task
 * @return "" means failed
 * */
OPTIONAL [[deprecated]] const char* getURL();
typedef const char* (FUNCTION_CALLER *GETURL)();

namespace webAPI {
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

}

namespace webAPI::formater {
 class PlatformFormater;

 /**
  * Plugin register formater, for frontend asked format platform's user prefers
  * @param adder may throw error (webAPI::formater::SameException), you may need try-catch
  */
 OPTIONAL void registerFormater(std::function<bool(const PlatformFormater&)> adder);
 typedef void (FUNCTION_CALLER *REGISTER_FORMATER)(std::function<bool(const PlatformFormater&)> adder);
}

#define URL_PARAMS_CATEGORY "category"
#define URL_PARAMS_PREPARED "prepared"
#define BODY_PARAMS_PLATFORM "platform"
#define BODY_PARAMS_NAME "name"
//#define URL_PARAMS_USERNAME "username"
//#define URL_PARAMS_PASSWORD "password"
#define URL_PARAMS_CLIENT_ID "id"
#define URL_PARAMS_TEST "test"
#define URL_PARAMS_SESSION "session"
#define BODY_PARAMS_ENCRYPT_KEY "key"
#define BODY_PARAMS_ADMIN "admin"