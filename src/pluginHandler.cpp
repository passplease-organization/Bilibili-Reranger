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

void PluginHandler::unloadAll() {
    if(!plugins -> empty())
        for(const auto & plugin : *plugins)
            plugin -> unload();
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
    auto plugin = (JUDGE) getFunction(webAPI::schedules::isPreCrawlThread() ? "judge" : "judgeAndRecommend");
    if(plugin == nullptr)
        return VideoStatus::UNKNOWN;
    return plugin(video,score);
}

void PluginHandler::tagVideo(const webAPI::Video &video, std::function<bool(const char *)> tagger, int) {
    auto plugin = (TAG_VIDEO) getFunction("tagVideo");
    if (plugin != nullptr)
        plugin(video,tagger);
}

string PluginHandler::getURL() {
    auto plugin = (GETURL) getFunction("getURL");
    if(plugin == nullptr)
        return "";
    return string(plugin());
}

webAPI::BrowseWorker PluginHandler::getWorker(crawlTask::Task* const& task) {
    auto plugin = (GETWORKER) getFunction("getWorker");
    if(plugin == nullptr)
        return webAPI::nullWorker();
    return plugin(task);
}

bool PluginHandler::dealJson(const Json& data,crawlTask::Task* const& task,vector<crawlTask::Task>& tempTasks,const unsigned short& workCount) {
    auto plugin = (DEAL_JSON) getFunction("pluginDealJson");
    if(plugin == nullptr)
        return false;
    return plugin(data,task,tempTasks,workCount);
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
        short count = 0;
        for(const auto & plugin : *plugins){
            switch(function(*plugin)){
                case VideoStatus::HARD_KEEP : return true;
                case VideoStatus::HARD_THROW : return false;
                case VideoStatus::SOFT_KEEP : count++;
                case VideoStatus::SOFT_THROW : count--;
                case VideoStatus::UNKNOWN : continue;
                default :
                    cppUtil::throwError("Invalid video status return value !");
            }
        }
        return count >= 0;
    }
    cppUtil::warn("没有安装插件，无法进行视频判断！");
    return true;
}

void PluginHandler::tagVideo(const webAPI::Video &video, std::function<bool(const char *)> tagger) {
    for(const auto & plugin : *plugins) {
        plugin -> tagVideo(video,tagger,0);
    }
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

bool PluginHandler::allDealJson(const Json &data, crawlTask::Task *const &task,const unsigned short& workCount){
    vector<crawlTask::Task> tempTasks;
    bool back = false;
    for(const auto& plugin : *plugins)
        back |= plugin -> dealJson(data,task,tempTasks,workCount);
    if (workCount < MAX_TEMP_TASK_WORK_COUNT)
        for(const auto& tempTask : tempTasks)
            back &= webAPI::schedules::registerTempScheduleTask(tempTask);
    return back;
}

void PluginHandler::exportEvent(Event::Event *const &event) {
    for(const auto& plugin : *plugins) {
        auto f = (EVENT_LISTENER)plugin -> getFunction("eventListener");
        if (f != nullptr)
            f(event);
    }
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

webAPI::BrowseWorker pluginGetWorker(crawlTask::Task* const& task) {
    auto back = PluginHandler::forEachPlugin<webAPI::BrowseWorker>(
            webAPI::nullWorker(),
            [&task](PluginHandler& handler) -> webAPI::BrowseWorker{
                return handler.getWorker(task);
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
