#pragma once
#include <vector>
#include <string>
#include "interface.h"

using namespace std;
using namespace crawlTask;
namespace fs = std::filesystem;

class PluginHandler {
    string name;
#ifdef WIN32
    HINSTANCE__* dll;
    #define DLL ".dll"
#elifdef __linux__
    void* dll;
    #define DLL ".so"
#endif
    const static string Plugin_Dir;
    static const vector<string>* pluginNames;
    static vector<PluginHandler*>* plugins;
public:
    explicit PluginHandler(const string& name);

    ~PluginHandler();

    void* getFunction(const string& function);

    PluginStatus load();

    PluginStatus registerGroups();

    VideoStatus roughJudge();

    VideoStatus judge();

    string getURL();

    bool dealJson(const string& tempdata);

    [[nodiscard]] const string& getName() const;

    static void forEachPlugin(PluginStatus function(PluginHandler&));

    /**
     * @param dealValue return value is to control stop or not
     * */
    template<typename T>
    static T forEachPlugin(T defaultValue, const function<T(PluginHandler &)> &function, const std::function<bool(T &back, T &now)> dealValue) {
        T value = defaultValue;
        if(!plugins -> empty()) {
            for(const auto & plugin : *plugins){
                T t = function(*plugin);
                if(dealValue(value,t))
                    return value;
            }
        }
        dealValue(value,value);
        return value;
    }

    static bool checkVideo(VideoStatus function(PluginHandler&));

    static void loadAll();

private:
    static vector<string>* searchPlugin(NotNull vector<string>* back);
};

bool roughCheckVideo();

bool finalCheckVideo();

bool pluginDealJson(string& tempData);

string pluginGetURL();