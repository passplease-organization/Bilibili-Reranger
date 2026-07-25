#pragma once
#include <vector>
#include <string>
#include "interface.h"

using namespace std;
using namespace crawlTask;
namespace fs = std::filesystem;

#define PluginDir "plugins"

namespace webAPI {
    class BrowseWorker;
}
class PluginHandler {
    string name;
#ifdef WIN32
    HINSTANCE__* dll;
    #define DLL ".dll"
#elifdef __linux__
    void* dll;
    #define DLL ".so"
#endif
    static const vector<string>* pluginNames;
    static vector<PluginHandler*>* plugins;

    friend int plugin(boost::asio::ip::tcp::socket& socket);
public:
    explicit PluginHandler(const string& name);

    ~PluginHandler();

    void* getFunction(const string& function);

    PluginStatus load();

    PluginStatus registerGroups();

    VideoStatus roughJudge(const webAPI::Video& video);

    VideoStatus judge(const webAPI::Video& video);

    [[deprecated]] string getURL();

    webAPI::BrowseWorker getWorker();

    bool dealJson(const string& tempdata);

    int dealRequest(boost::asio::ip::tcp::socket& socket,const Json& data);

    void registerFormater(std::function<bool(const webAPI::formater::PlatformFormater&)> adder);

    void feedBack(const webAPI::formater::FeedBack& feedback);

    bool registerScheduleTasks();

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

    static bool checkVideo(const std::function<VideoStatus(PluginHandler &)> &function);

    static void loadAll();

    static int handleRequest(boost::asio::ip::tcp::socket& socket,const string& name,const Json& data);

    static vector<webAPI::formater::PlatformFormater> getFormater(const string& platform);

    static void allFeedBack(const webAPI::formater::FeedBack& feedback) {
        for(const auto& plugin : *plugins)
            plugin -> feedBack(feedback);
    }

    static bool registerAllScheduleTasks() noexcept(false){
        for(const auto& plugin : *plugins)
            if (!plugin -> registerScheduleTasks())
                webAPI::schedules::throwRegisterError();
        return true;
    }

private:
    static vector<string>* searchPlugin(NotNull vector<string>* back);
};

bool roughCheckVideo(const webAPI::Video& video);

bool finalCheckVideo(const webAPI::Video& video);

bool pluginDealJson(string& tempData);

[[deprecated]] string pluginGetURL();

webAPI::BrowseWorker pluginGetWorker();