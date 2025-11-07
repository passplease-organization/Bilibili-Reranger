#pragma once
#include "pluginInterface.h"

#pragma once

#define REQUIRED
#define OPTIONAL

#ifdef WIN32
   #define FUNCTION_CALLER __cdecl
#elifdef __linux__
   #define FUNCTION_CALLER
#endif

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
 * Rough judge whether need to crawl more data , keep it in website or skip this video
 * */
OPTIONAL VideoStatus roughJudge();
typedef VideoStatus (FUNCTION_CALLER *ROUGH_JUDGE)();

/**
 * Judge keep or throw this video away
 * */
OPTIONAL VideoStatus judge();
typedef VideoStatus (FUNCTION_CALLER *JUDGE)();

/**
 * Get this specific url for now task
 * @return "" means failed
 * */
OPTIONAL const char* getURL();
typedef const char* (FUNCTION_CALLER *GETURL)();

/**
 * Deal this specific json for now crawling
 * @return true means crawl succeed
 * */
OPTIONAL bool dealJson(const char* data);
typedef bool (FUNCTION_CALLER *DEAL_JSON)(const char* data);