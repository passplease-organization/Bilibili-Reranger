#include "pluginInterface.h"
#include "config.h"

using namespace crawlTask;

vector<Group*> groups = vector<Group*>();
unsigned int workingOn = 0;

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

Nullable Group* crawlTask::getGroup(const char* groupName) noexcept{
    if(groupName != nullptr) {
        string name(groupName);
        for (const auto group: groups) {
            if (group->isName(groupName))
                return group;
        }
        return nullptr;
    }
    if(validIndex())
        return groups[workingIndex()];
    //            string error("Wrong working Index of ");
    //            error += std::to_string(workingIndex());
    //            error += ", but groups size is ";
    //            error += std::to_string(groups.size());
    //            throwError(error.c_str());
    return nullptr;
}

Nullable Group* crawlTask::nextGroup(){
    workingOn++;
    return validIndex(workingIndex()) ? getGroup() : nullptr;
}

bool crawlTask::registerTask(const char* groupName,Task* task,bool create){
    auto group = getGroup(groupName);
    if(group == nullptr && create){
        Group temp = Group(groupName,task -> videoCount);
        group = &temp;
    }
    if(group == nullptr) {
        throwError("Register task failed due to get group failed");
        return false;
    }
    return group -> registerTask(task);
}

string groupFilter;
void crawlTask::GroupFilter(const string &target) {
    groupFilter = target;
}


bool crawlTask::registerGroup(Group *group, const char *groupName) {
    if (!groupFilter.empty() && (groupName == nullptr || groupName != groupFilter) && group -> name != groupFilter)
        return true;
    if(groupName == nullptr){
        groupName = group -> name;
        goto reg;
    }
    if(!group -> isName(groupName))
        group -> name = groupName;

    reg:
    auto exists = getGroup(groupName);
    if(exists == nullptr) {
        groups.emplace_back(group);
    }else {
        *exists += group;
    }
    return getGroup(groupName) != nullptr;
}

Task* Group::nextTask(bool move) {
    if(validIndex(workingIndex + 1)){
        workingIndex += move;
        return tasks[workingIndex];
    }
    workingIndex += move;
    return nullptr;
}

Task *Group::nowTask() {
    if(validIndex())
        return tasks[workingIndex];
    string error("Wrong working Index of ");
    error += std::to_string(workingIndex);
    error += ", but tasks size is ";
    error += std::to_string(tasks.size());
    throwError(error.c_str());
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

Group::Group(const char *name,unsigned int videoCount,bool regi) {
    this -> name = name;
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
    data.put("videoCount",&group -> videoCount);
    for(auto& task : group -> tasks){
        dataStore::Data a{};
        task_to_data(a,task);
        data.put("tasks",&a,true);
    }
}
