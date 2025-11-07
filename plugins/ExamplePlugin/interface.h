#pragma once
#include "pluginInterface.h"

#ifdef WIN32
    #define EXPORT __declspec(dllexport)
#elifdef __linux__
    #define EXPORT __attribute__((visibility("default")))
#endif

extern "C" {
EXPORT PluginStatus load();

EXPORT void registerGroups();

EXPORT VideoStatus roughJudge();

EXPORT VideoStatus judge();

#ifdef DEVELOP
EXPORT const char* getURL();

EXPORT bool dealJson(const char* data);
#endif
}
