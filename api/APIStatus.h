#pragma once
#ifdef API_DLL
#ifdef WIN32
    #define API __declspec(dllexport)
#elifdef __linux__
    #define API __attribute__((visibility("default")))
#endif

#else
#ifdef WIN32
    #define API __declspec(dllimport)
#elifdef __linux__
    #define API __attribute__((visibility("default")))
#endif
#endif

#define Nullable /*This can be null*/
#define NotNull /*This can't be null*/
