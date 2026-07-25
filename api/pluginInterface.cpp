#include "pluginInterface.h"
#include "utils/config.h"
#include <cstring>
#ifdef TEST
    #include "webAPIs/platforms.h"
#endif

using namespace crawlTask;

namespace {
    using GroupKey = pair<string,string>;

    GroupKey makeGroupKey(const char* name,const char* platform) {
        return {name == nullptr ? "" : name, platform == nullptr ? "" : platform};
    }

    char* duplicateCString(const char* value) {
        const char* source = value == nullptr ? "" : value;
        const size_t size = strlen(source) + 1;
        char* buffer = nullptr;
        if (size <= MAX_BUFFER_SIZE)
            defaultOutputChar(&buffer);
        else
            buffer = new char[size];
        memcpy(buffer, source, size);
        return buffer;
    }

    void releaseCString(const char*& value) {
        if (value == nullptr)
            return;
        auto* buffer = const_cast<char*>(value);
        freeOutputChar(&buffer);
        value = nullptr;
    }

    void replaceCString(const char*& target,const char* value) {
        releaseCString(target);
        target = duplicateCString(value);
    }
}

thread_local vector<Group*> groups = vector<Group*>();
thread_local map<GroupKey,Group*> groupIndex = map<GroupKey,Group*>();
thread_local unsigned int workingOn = 0;

Task::Task(const char *keyword,unsigned int videoCount, WorkingMode mode,int publishedDay) {
    _workCount = 0;
    this -> keyword = duplicateCString(keyword);
    this -> mode = mode;
    this -> videoCount = (int)videoCount;
    this -> publishedDay = publishedDay >= 0 ? publishedDay : defaultDaytime(mode);
}

Task::~Task() {
    releaseCString(keyword);
}

const char* crawlTask::getName(WorkingMode mode){
    switch (mode) {
        case WorkingMode::SEARCH: {
            return "搜索模式";
        }
        case WorkingMode::SUBSCRIBE: {
            return "关注列表匹配模式";
        }
        case WorkingMode::CATEGORY: {
            return "分类";
        }
        case WorkingMode::TAG: {
            return "视频标签匹配模式";
        }
        case WorkingMode::HOME_PAGE_FILTER: {
            return "主页筛选";
        }
        default: {
            cppUtil::throwError("Unknown WorkingMode Type !");
        }
    }
}

int crawlTask::defaultDaytime(crawlTask::WorkingMode mode) {
    switch(mode){
        case WorkingMode::SUBSCRIBE : return config<int>(SUBSCRIBE_PUBLISH_TIME);
        default : return INT_MAX;
    }
}

WorkingMode crawlTask::byName(const char *name) {
    string mode(name);
    if(mode == "搜索模式")
        return WorkingMode::SEARCH;
    if(mode == "关注列表匹配模式")
        return WorkingMode::SUBSCRIBE;
    if (mode == "分类")
        return WorkingMode::CATEGORY;
    if(mode == "视频标签匹配模式")
        return WorkingMode::TAG;
    if(mode == "主页筛选")
        return WorkingMode::HOME_PAGE_FILTER;
    cppUtil::warn("未匹配的模式名称");
    return WorkingMode::SEARCH;
}

Nullable Group* crawlTask::getGroup(const char* groupName,const char* platform) noexcept{
    if (groups.empty())
        return nullptr;
    if (groupName == nullptr || platform == nullptr) {
        return groups[workingOn];
    }
    if (const auto& key = makeGroupKey(groupName,platform); groupIndex.contains(key))
        return groupIndex[key];
    return nullptr;
}

Nullable Group* crawlTask::nextGroup(){
    workingOn++;
    return validIndex(workingIndex()) ? getGroup() : nullptr;
}

bool crawlTask::registerTask(const char* groupName,const char* platform,Task* task,bool create){
    auto group = getGroup(groupName,platform);
    if(group == nullptr && create){
        auto temp = Group(groupName,platform,task -> videoCount);
        group = &temp;
        registerGroup(group);
    }
    if(group == nullptr)
        cppUtil::throwError("Register task failed due to get group failed");
    return group -> registerTask(task);
}

thread_local GroupKey groupFilter;
void crawlTask::GroupFilter(const string &target,const string& platform) {
    groupFilter = makeGroupKey(target.c_str(),platform.c_str());
}


bool crawlTask::registerGroup(Group *group, const char *groupName,const char* platform) {
    const char* resolvedName = groupName == nullptr ? group -> name : groupName;
    const char* resolvedPlatform = platform == nullptr ? group -> platform : platform;
    const bool filterActive = !groupFilter.first.empty() && !groupFilter.second.empty();
    if (resolvedName == nullptr || resolvedPlatform == nullptr)
        return false;
#ifdef TEST
    if (filterActive && (resolvedName != groupFilter.first || (resolvedPlatform != groupFilter.second && groupFilter.second != ALL_PLATFORMS))){
#else
    if (filterActive && (resolvedName != groupFilter.first || resolvedPlatform != groupFilter.second)) {
#endif
        return true;
    }

    const auto key = makeGroupKey(group -> name, group -> platform);
    if(groupIndex.contains(key)) {
        *groupIndex[key] += group;
        groups.push_back(group);
    }else {
        groupIndex[key] = group;
        groups.emplace_back(group);
    }
    return groupIndex.contains(key);
}

Task* Group::nextTask(bool move) {
    if(validIndex(workingIndex + 1)){
        workingIndex += move;
        return tasks[workingIndex];
    }
    workingIndex += move;
    return nullptr;
}

Task *Group::nowTask() const{
    if(validIndex())
        return tasks[workingIndex];
    return nullptr;
}

bool Group::validIndex() const {
    return validIndex(workingIndex);
}

bool Group::validIndex(unsigned int index) const {
    return index < tasks.size();
}

bool Group::registerTask(Task *task) {
    if(task == nullptr)
        return false;
    tasks.emplace_back(task);
    videoCount += task -> videoCount;
    return true;
}

Group::Group(const char *name,const char* platform,unsigned int videoCount,bool regi) {
    this -> name = duplicateCString(name);
    this -> platform = duplicateCString(platform);
    this -> videoCount = (int) videoCount;
    if(regi && !registerGroup(this, name)){
        string error = "Register group failed ! Group name: ";
        error += name;
        cppUtil::throwError(error);
    }
}

Group::~Group() {
    releaseCString(name);
    releaseCString(platform);
    for (auto* task : tasks)
        delete task;
    tasks.clear();
}

Group *Group::operator+=(crawlTask::Group &other) {
    if(isName(other.name)){
        tasks.insert(tasks.end(),other.tasks.begin(), other.tasks.end());
        videoCount += other.videoCount;
    }
    return this;
}

Group *Group::operator+=(crawlTask::Group* other) {
    if(isName(other -> name)){
        tasks.insert(tasks.end(),other -> tasks.begin(), other -> tasks.end());
        videoCount += other -> videoCount;
    }
    return this;
}

bool Group::isName(const char *compare) const {
    string a(name);
    string b(compare);
    return a == b;
}

Nullable Task *crawlTask::nextTask(bool move) {
    auto group = getGroup();
    if(group != nullptr){
        auto next = group -> nextTask(move);
        if(next == nullptr){
            auto _next = nextGroup();
            if(_next == nullptr)
                return nullptr;
            return _next -> nowTask();
        }
        return next;
    }
    return nullptr;
}

Task *crawlTask::nowTask() noexcept(false){
    if (!validIndex(workingIndex()))
        return nullptr;
    if(auto group = getGroup(); group != nullptr){
        return group -> nowTask();
    }
    //        throwError("Wrong working status !");
    return nullptr;
}

unsigned int crawlTask::workingIndex() {
    return workingOn;
}

bool crawlTask::validIndex(unsigned int index){
    return  index < groups.size();
}

void crawlTask::task_from_data(dataStore::Data &data, crawlTask::Task* task) {
    string keyword = data.value("keyword", "");
    replaceCString(task -> keyword, keyword.c_str());
    task -> videoCount = data.value("videoCount", 0);
    if(data.contains("working_mode") && data["working_mode"].is_string()) {
        const string mode = data["working_mode"];
        task -> mode = byName(mode.c_str());
    }else {
        int mode = data.value("working_mode", 0);
        task -> mode = static_cast<WorkingMode>(mode);
    }
}

void crawlTask::task_to_data(dataStore::Data& data,const Task* task){
    data["keyword"] = task -> keyword;
    data["working_mode"] = getName(task -> mode);
    data["videoCount"] = task -> videoCount;
}

void crawlTask::group_from_data(dataStore::Data& data, Group* group){
    string name = data.value("name", "");
    replaceCString(group -> name, name.c_str());
    if (!data.contains("platform") || !data["platform"].is_string()) {
        cppUtil::throwError("Missing platform in group data");
    }
    string platform = data["platform"];
    replaceCString(group -> platform, platform.c_str());
    group -> videoCount = data.value("videoCount", 0);
    for(auto& task : data.value("tasks", dataStore::Data::array())){
        Task* t = new Task("",0);
        task_from_data(task,t);
        if(!string(t -> keyword).empty())
            group -> tasks.emplace_back(t);
        else {
            delete t;
            cppUtil::throwError("Wrong data format for tasks");
        }
    }
}

void crawlTask::group_to_data(dataStore::Data& data, const Group* group){
    data["name"] = group -> name;
    data["platform"] = group -> platform;
    data["videoCount"] = group -> videoCount;
    data["tasks"] = dataStore::Data::array();
    for(auto& task : group -> tasks){
        dataStore::Data a{};
        task_to_data(a,task);
        data["tasks"].push_back(a);
    }
}

const vector<Group *>& crawlTask::getAllGroups() {
    return groups;
}

