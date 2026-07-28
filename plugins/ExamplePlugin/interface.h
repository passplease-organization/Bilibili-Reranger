#pragma once
#include "pluginInterface.h"

#ifdef WIN32
    #define EXPORT __declspec(dllexport)
#elifdef __linux__
    #define EXPORT __attribute__((visibility("default")))
#endif

namespace webAPI {
    class Video;
}

extern "C" {
EXPORT PluginStatus load();

EXPORT void registerGroups();

EXPORT VideoStatus roughJudge(const webAPI::Video& video);

EXPORT VideoStatus judge(const webAPI::Video& video,unsigned short& score);

#ifdef DEVELOP
EXPORT const char* getURL();

EXPORT bool dealJson(const char* data);
#endif
}
