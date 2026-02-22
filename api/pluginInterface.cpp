#include "pluginInterface.h"
#include "config.h"
#ifdef TEST
    #include "webAPIs/platforms.h"
#endif

using namespace crawlTask;

namespace {
    using GroupKey = pair<string,string>;

    GroupKey makeGroupKey(const char* name,const char* platform) {
        return {name == nullptr ? "" : name, platform == nullptr ? "" : platform};
    }
}

thread_local vector<Group*> groups = vector<Group*>();
thread_local map<GroupKey,Group*> groupIndex = map<GroupKey,Group*>();
thread_local unsigned int workingOn = 0;

Task::Task(const char *keyword,unsigned int videoCount, WorkingMode mode,int publishedDay) {
    this -> keyword = keyword;
    this -> mode = mode;
    this -> videoCount = (int)videoCount;
    this -> publishedDay = publishedDay >= 0 ? publishedDay : defaultDaytime(mode);
}

const char* crawlTask::getName(WorkingMode mode){
    switch (mode) {
        case WorkingMode::SEARCH: {
            return "搜索模式";
        }
        case WorkingMode::SUBSCRIBE: {
            return "关注列表匹配模式";
        }
        case WorkingMode::TAG: {
            return "视频标签匹配模式";
        }
        default: {
            throwError("Unknown WorkingMode Type !");
            return "Error";
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
    if(mode == "视频标签匹配模式")
        return WorkingMode::TAG;
    warn("未匹配的模式名称");
    return WorkingMode::SEARCH;
}

Nullable Group* crawlTask::getGroup(const char* groupName,const char* platform) noexcept{
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
    if(group == nullptr) {
        throwError("Register task failed due to get group failed");
        return false;
    }
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
    if (filterActive && resolvedName != groupFilter.first || (resolvedPlatform != groupFilter.second && groupFilter.second != ALL_PLATFORMS)){
#else
    if (filterActive && resolvedName != groupFilter.first || resolvedPlatform != groupFilter.second) {
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
    this -> name = name;
    this -> platform = platform;
    this -> videoCount = (int) videoCount;
    if(regi && !registerGroup(this, name)){
        string error = "Register group failed ! Group name: ";
        error += name;
        throwError(error.c_str());
    }
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
    auto group = getGroup();
    if(group != nullptr){
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
    data.get("keyword",&task -> keyword);
    int* count;
    data.get("videoCount",&count);
    task -> videoCount = *count;
    if(data.strings.contains("working_mode")) {
        const char *mode = nullptr;
        data.get("working_mode", &mode);
        task -> mode = byName(mode);
    }else {
        int* mode = nullptr;
        data.get("working_mode",&mode);
        task -> mode = static_cast<WorkingMode>(*mode);
    }
}

void crawlTask::task_to_data(dataStore::Data& data,const Task* task){
    data.put("keyword",task -> keyword);
    data.put("working_mode", getName(task -> mode));
    data.put("videoCount",&task -> videoCount);
}

void crawlTask::group_from_data(dataStore::Data& data, Group* group){
    data.get("name",&group -> name);
    if (!data.strings.contains("platform")) {
        throwError("Missing platform in group data");
    }
    data.get("platform",&group -> platform);
    vector<dataStore::Data>* datas;
    data.get("tasks",&datas);
    int* count;
    data.get("videoCount",&count);
    group -> videoCount = *count;
    for(auto& task : *datas){
        Task* t = new Task("",0);
        task_from_data(task,t);
        if(!string(t -> keyword).empty())
            group -> tasks.emplace_back(t);
        else {
            delete t;
            throwError("Wrong data format for tasks");
        }
    }
}

void crawlTask::group_to_data(dataStore::Data& data, const Group* group){
    data.put("name",group -> name);
    data.put("platform",group -> platform);
    data.put("videoCount",&group -> videoCount);
    for(auto& task : group -> tasks){
        dataStore::Data a{};
        task_to_data(a,task);
        data.put("tasks",&a,true);
    }
}

const vector<Group *>& crawlTask::getAllGroups() {
    return groups;
}

