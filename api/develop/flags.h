#pragma once

#if DEVELOP
#define TEST_DLL false
#define CONNECT_INTERNET true
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
#define SLEEP_CRAWL true
#define NEED_PORT true
#define EASY_LOGIN false
#endif

#if CONNECT_INTERNET
#else
    #ifdef WIN32
        #define BILIBILI_DATA ".\\testing\\DataFromBilibili.json"
    #elifdef __linux__
        #define BILIBILI_DATA "./testing/DataFromBilibili.json"
    #endif
#endif