#pragma once

#if DEVELOP
#define TEST_DLL false
#define ALL_CONTAINER_ONLINE true
#define MORE_DETAILS true
#define SLEEP_CRAWL false
#define tempDataPath "tempData"
#define tempDataName tempDataPath
    #ifdef TEST
        #define NEED_PORT true
        #define EASY_LOGIN true
    #else
        #define NEED_PORT true // TODO Delete it for some time
        #define EASY_LOGIN false
    #endif
#else
#define CONNECT_INTERNET true
#define ALL_CONTAINER_ONLINE true
#define SLEEP_CRAWL true
#define NEED_PORT true
#define EASY_LOGIN false
#define MORE_DETAILS false
#endif

#if ALL_CONTAINER_ONLINE
#else
    #ifdef WIN32
        #define BILIBILI_DATA ".\\testing\\DataFromBilibili.json"
    #elifdef __linux__
        #define BILIBILI_DATA "./testing/DataFromBilibili.json"
    #endif
#endif