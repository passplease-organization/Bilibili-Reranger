#include "PluginHandler.h"
#include <iostream>

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
    this -> name = std::move(name);
    #ifdef WIN32
        SetDllDirectoryA(Plugin_Dir.c_str());
        dll = LoadLibrary(TEXT((this -> name + DLL).c_str()));
    #elifdef __linux__
        dll = dlopen((Plugin_Dir + "/" + this -> name + DLL).c_str(),RTLD_LAZY);
    #endif
    if(dll == nullptr) {
        say("尝试寻找插件：",false);
        say(this -> name.c_str(),false);
        say("失败");
        throwError("未找到插件库！");
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
    say("正在加载插件：",false);
    say(getName().c_str());
    auto createPlugin = (LOAD) getFunction("load");
    if(createPlugin == nullptr){
        say("未找到插件方法！");
        return PluginStatus::FAIL;
    }
    PluginStatus value = createPlugin();
    if(value == PluginStatus::FAIL){
        say("插件注册失败！");
        return PluginStatus::FAIL;
    }
    say("插件加载成功");
    return value;
}

PluginStatus PluginHandler::registerGroups() {
    auto plugin = (REGISTER) getFunction("registerGroups");
    if(plugin == nullptr)
        return PluginStatus::PASS;
    plugin();
    return PluginStatus::SUCCESS;
}

VideoStatus PluginHandler::roughJudge() {
    auto plugin = (ROUGH_JUDGE) getFunction("roughJudge");
    if(plugin == nullptr)
        return VideoStatus::UNKNOWN;
    return plugin();
}

VideoStatus PluginHandler::judge() {
    auto plugin = (JUDGE) getFunction("judge");
    if(plugin == nullptr)
        return VideoStatus::UNKNOWN;
    return plugin();
}

string PluginHandler::getURL() {
    auto plugin = (GETURL) getFunction("getURL");
    if(plugin == nullptr)
        return "";
    return string(plugin());
}

bool PluginHandler::dealJson(const string &tempdata) {
    auto plugin = (DEAL_JSON) getFunction("dealJson");
    if(plugin == nullptr)
        return false;
    return plugin(tempdata.c_str());
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
                    say("插件运行失败！请检查具体原因！");
                    break;
                }
                case PluginStatus::SUCCESS :
                case PluginStatus::PASS : continue;
                default :
                    throwError("Invalid plugin return value !");
            }
        }
    }
}

bool PluginHandler::checkVideo(VideoStatus function(PluginHandler &)) {
    if(!plugins -> empty()) {
        for(const auto & plugin : *plugins){
            switch(function(*plugin)){
                case VideoStatus::KEEP : return true;
                case VideoStatus::THROW : return false;
                case VideoStatus::UNKNOWN : continue;
                default :
                    throwError("Invalid video status return value !");
            }
        }
    }
    return true;
}

void PluginHandler::loadAll() {
    say("插件加载完成，共发现",false);
    say(to_string(pluginNames -> size()).c_str(),false);
    say("个插件");
    if(!pluginNames -> empty()) {
        for(const auto & plugin : *pluginNames){
            auto examplePlugin = new PluginHandler(plugin);
            switch(examplePlugin -> load()){
                case PluginStatus::FAIL :
                    cout << plugin;
                    say("插件运行失败！请检查具体原因！");
                    break;
                case PluginStatus::SUCCESS :
                    plugins -> emplace_back(examplePlugin);
                    continue;
                case PluginStatus::PASS :
                    break;
                default :
                    throwError("Invalid plugin return value !");
            }
            delete examplePlugin;
        }
    }
}

vector<string> *PluginHandler::searchPlugin(vector<string> *back) {
    string path = string();
#ifdef WIN32
    path.append(".\\").append(Plugin_Dir);
#elifdef __linux__
    path.append("./").append(Plugin_Dir);
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
                say("找到插件：",false);
                say(fileName.c_str());
            }
        }
    }else say("插件寻找结果：未找到插件");
    say("插件加载结束，即将退出插件加载进程");
    return back;
}

const string PluginHandler::Plugin_Dir = "plugins";

const vector<string>* PluginHandler::pluginNames = PluginHandler::searchPlugin(new vector<string>());

vector<PluginHandler*>* PluginHandler::plugins = new vector<PluginHandler*>();

bool roughCheckVideo() {
    return PluginHandler::checkVideo([](PluginHandler& handler) -> VideoStatus{
        return handler.roughJudge();
    });
}

bool finalCheckVideo() {
    return PluginHandler::checkVideo([](PluginHandler& handler) -> VideoStatus{
        return handler.judge();
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