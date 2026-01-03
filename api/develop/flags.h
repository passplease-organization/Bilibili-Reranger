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
    #else
        #define NEED_PORT true
    #endif
#else
#define CONNECT_INTERNET true
#define SLEEP_CRAWL true
#define NEED_PORT true
#endif

#if CONNECT_INTERNET
#else
    #ifdef WIN32
        #define BILIBILI_DATA ".\\testing\\DataFromBilibili.json"
    #elifdef __linux__
        #define BILIBILI_DATA "./testing/DataFromBilibili.json"
    #endif
#endif
