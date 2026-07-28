#include "PluginHandler.h"
#include "utils/Util.h"
#include <iostream>
#include "utils/config.h"
#include "exit.h"
#include "webAPIs/browse.h"
#include "webAPIs/frontend/PlatformFormater.h"

#ifdef WIN32
    #include <minwindef.h>
    #include <windows.h>
#elifdef __linux__
    #include <dlfcn.h>
#endif

using namespace std;
using namespace crawlTask;
namespace fs = std::filesystem;

PluginHandler::PluginHandler(const string &name) {
    this -> name = name;
    #ifdef WIN32
        SetDllDirectoryA(PluginDir);
        dll = LoadLibrary(TEXT((this -> name + DLL).c_str()));
    #elifdef __linux__
        dll = dlopen((PluginDir "/" + this -> name + DLL).c_str(),RTLD_LAZY);
    #endif
    if(dll == nullptr) {
        cppUtil::say({false, nullptr}, "尝试寻找插件：");
        cppUtil::say({false, nullptr}, this -> name);
        cppUtil::say("失败");
        cppUtil::throwError("未找到插件库！");
    }
}

PluginHandler::~PluginHandler() {
    #ifdef WIN32
        FreeLibrary(dll);
    #elifdef __linux__
        dlclose(dll);
    #endif
}

void *PluginHandler::getFunction(const string &function) {
    #ifdef WIN32
        return GetProcAddress(dll,function.c_str());
    #elifdef __linux__
        return dlsym(dll,function.c_str());
    #endif
}

PluginStatus PluginHandler::load() {
    cppUtil::say({false, nullptr}, "正在加载插件：");
    cppUtil::say(getName());
    auto createPlugin = (LOAD) getFunction("load");
    if(createPlugin == nullptr){
        cppUtil::say("未找到插件方法！");
        return PluginStatus::FAIL;
    }
    PluginStatus value = createPlugin();
    if(value == PluginStatus::FAIL){
        cppUtil::say("插件注册失败！");
        return PluginStatus::FAIL;
    }
    cppUtil::say("插件加载成功");
    return value;
}

void PluginHandler::unload() {
    cppUtil::say("正在卸载插件",getName());
    if(const auto plugin = (UNLOAD) getFunction("unload");plugin != nullptr)
        plugin();
}

PluginStatus PluginHandler::registerGroups() {
    auto plugin = (REGISTER) getFunction("registerGroups");
    if(plugin == nullptr)
        return PluginStatus::PASS;
    plugin();
    return PluginStatus::SUCCESS;
}

VideoStatus PluginHandler::roughJudge(const webAPI::Video& video) {
    auto plugin = (ROUGH_JUDGE) getFunction("roughJudge");
    if(plugin == nullptr)
        return VideoStatus::UNKNOWN;
    return plugin(video);
}

VideoStatus PluginHandler::judge(const webAPI::Video& video,unsigned short& score) {
    auto plugin = (JUDGE) getFunction("judge");
    if(plugin == nullptr)
        return VideoStatus::UNKNOWN;
    return plugin(video,score);
}

string PluginHandler::getURL() {
    auto plugin = (GETURL) getFunction("getURL");
    if(plugin == nullptr)
        return "";
    return string(plugin());
}

webAPI::BrowseWorker PluginHandler::getWorker() {
    auto plugin = (GETWORKER) getFunction("getWorker");
    if(plugin == nullptr)
        return webAPI::nullWorker();
    return plugin();
}

bool PluginHandler::dealJson(const string &tempdata) {
    auto plugin = (DEAL_JSON) getFunction("dealJson");
    if(plugin == nullptr)
        return false;
    return plugin(tempdata.c_str());
}

int PluginHandler::dealRequest(boost::asio::ip::tcp::socket &socket,const Json& data) {
    auto plugin = (DEAL_REQUEST) getFunction("dealRequest");
    if (plugin == nullptr)
        return success();
    return plugin(socket,data);
}

void PluginHandler::registerFormater(std::function<bool(const webAPI::formater::PlatformFormater&)> adder) {
    auto plugin = (webAPI::formater::REGISTER_FORMATER) getFunction("registerFormater");
    if (plugin == nullptr)
        return;
    plugin(adder);
}

void PluginHandler::feedBack(const webAPI::formater::FeedBack &feedback) {
    auto plugin = (FEED_BACK) getFunction("feedback");
    if (plugin == nullptr)
        return;
    plugin(feedback);
}

bool PluginHandler::registerScheduleTasks() {
    auto plugin = (ScheduleCrawl) getFunction("scheduleCrawl");
    if(plugin == nullptr)
        return true;
    for (const auto& t : plugin())
        if (!webAPI::schedules::registerScheduleTask(t))
            return false;
    return true;
}

const string &PluginHandler::getName() const {
    return this -> name;
}

void PluginHandler::forEachPlugin(PluginStatus function(PluginHandler &)) {
    if(!plugins -> empty()) {
        for(const auto & plugin : *plugins){
            switch(function(*plugin)){
                case PluginStatus::FAIL : {
                    cout << plugin;
                    cppUtil::say("插件运行失败！请检查具体原因！");
                    break;
                }
                case PluginStatus::SUCCESS :
                case PluginStatus::PASS : continue;
                default :
                    cppUtil::throwError("Invalid plugin return value !");
            }
        }
    }
}

bool PluginHandler::checkVideo(const std::function<VideoStatus(PluginHandler &)> &function) {
    if(!plugins -> empty()) {
        for(const auto & plugin : *plugins){
            switch(function(*plugin)){
                case VideoStatus::KEEP : return true;
                case VideoStatus::THROW : return false;
                case VideoStatus::UNKNOWN : continue;
                default :
                    cppUtil::throwError("Invalid video status return value !");
            }
        }
    }
    return true;
}

void PluginHandler::loadAll() {
    cppUtil::say({false, nullptr}, "插件加载完成，共发现");
    cppUtil::say({false, nullptr}, pluginNames -> size());
    cppUtil::say("个插件");
    if(!pluginNames -> empty()) {
        for(const auto & plugin : *pluginNames){
            auto examplePlugin = new PluginHandler(plugin);
            switch(examplePlugin -> load()){
                case PluginStatus::FAIL :
                    cout << plugin;
                    cppUtil::say("插件运行失败！请检查具体原因！");
                    break;
                case PluginStatus::SUCCESS :
                    plugins -> emplace_back(examplePlugin);
                    continue;
                case PluginStatus::PASS :
                    break;
                default :
                    cppUtil::throwError("Invalid plugin return value !");
            }
            delete examplePlugin;
        }
    }
}

int PluginHandler::handleRequest(boost::asio::ip::tcp::socket &socket, const string &name, const Json& data) {
    for (const auto& plugin : *plugins) {
        if (plugin -> getName() == name)
            return plugin -> dealRequest(socket,data);
    }
    return success();
}

vector<webAPI::formater::PlatformFormater> PluginHandler::getFormater(const string& platform) {
    auto formater = vector<webAPI::formater::PlatformFormater>();
    auto adder = [&formater,&platform](const webAPI::formater::PlatformFormater& f) -> bool {
        for (const auto& format : formater)
            format.notSame(f);
        if (f.platform == platform || platform.empty())
            formater.push_back(f);
        return true;
    };
    for(const auto& plugin : *plugins)
        plugin -> registerFormater(adder);
    return formater;
}

vector<string> *PluginHandler::searchPlugin(vector<string> *back) {
    string path = string();
#ifdef WIN32
    path.append(".\\").append(PluginDir);
#elifdef __linux__
    path.append("./").append(PluginDir);
#endif
    if(fs::exists(fs::absolute(path))){
        string fileName;
        for(auto& fileInfo : filesystem::directory_iterator(path)){
            fileName = fileInfo.path().filename().string();
            if(endWith(fileName.c_str(),DLL) && !filesystem::is_directory(fileInfo.path())){
                char* buffer = nullptr;
                removeEnd(fileName.c_str(),DLL,&buffer);
                back -> emplace_back(buffer);
                freeOutputChar(&buffer);
                buffer = nullptr;
                cppUtil::say(fileName);
            }
        }
    }else cppUtil::say("插件寻找结果：未找到插件");
    cppUtil::say("插件加载结束，即将退出插件加载进程");
    return back;
}

const vector<string>* PluginHandler::pluginNames = PluginHandler::searchPlugin(new vector<string>());

vector<PluginHandler*>* PluginHandler::plugins = new vector<PluginHandler*>();

bool roughCheckVideo(const webAPI::Video& video) {
    return PluginHandler::checkVideo([&video](PluginHandler& handler) -> VideoStatus{
        return handler.roughJudge(video);
    });
}

bool finalCheckVideo(const webAPI::Video& video,unsigned short& score) {
    return PluginHandler::checkVideo([&video,&score](PluginHandler& handler) -> VideoStatus{
        return handler.judge(video,score);
    });
}

bool pluginDealJson(string& tempData) {
    return PluginHandler::forEachPlugin<bool>(
            false,
            [tempData](PluginHandler& handler) -> bool{
                return handler.dealJson(tempData);
            },[](bool& back,bool& now) -> bool{
                back |= now;
                return back;
            }
    );
}

string pluginGetURL() {
    auto back = PluginHandler::forEachPlugin<string>(
            "",
            [](PluginHandler& handler) -> string{
                return handler.getURL();
            },[](string& back,string& now) -> bool{
                if(now.empty())
                    return true;
                else {
                    back = now;
                    return false;
                }
            }
    );
    return back;
}

webAPI::BrowseWorker pluginGetWorker() {
    auto back = PluginHandler::forEachPlugin<webAPI::BrowseWorker>(
            webAPI::nullWorker(),
            [](PluginHandler& handler) -> webAPI::BrowseWorker{
                return handler.getWorker();
            },[](webAPI::BrowseWorker& back,webAPI::BrowseWorker& now) -> bool{
                if(now == webAPI::nullWorker())
                    return true;
                else {
                    back = now;
                    return false;
                }
            }
    );
    return back;
}
