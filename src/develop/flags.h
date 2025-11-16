#pragma once

#if DEVELOP
#define TEST_DLL false
#define CONNECT_INTERNET true
#define MORE_DETAILS true
#define SLEEP_CRAWL false
#define tempDataPath "tempData"
#define tempDataName tempDataPath
#define NEED_PORT true
#elif
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
