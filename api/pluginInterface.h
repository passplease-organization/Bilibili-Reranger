#pragma once

#include "APIStatus.h"
#include "Util.h"

#pragma once

extern "C" {

enum struct PluginStatus {
    FAIL,
    SUCCESS,
    PASS
};

enum struct VideoStatus {
    KEEP,
    THROW,
    UNKNOWN
};

namespace crawlTask{
    enum struct WorkingMode {
        SEARCH,
        SUBSCRIBE,
        TAG
    };

    API int defaultDaytime(WorkingMode mode);

    API const char* getName(WorkingMode mode);

    API Nullable WorkingMode byName(const char* name);

    struct API Task{
    public:
        const char* keyword;
        WorkingMode mode;
        int videoCount;
        int publishedDay;
        Task(const char* keyword,unsigned int videoCount,WorkingMode mode = WorkingMode::SEARCH,int publishedDay = -1);
    };

    class Group{
    private:
        unsigned int workingIndex = 0;
    public:
        vector<Task*> tasks = vector<Task*>();
        const char* name;
        int videoCount;

        API explicit Group(const char* name,unsigned int videoCount = 0,bool regi = false);

        API Group* operator+= (Group& other);

        API Group* operator+= (Group* other);

        /**
         * @param move Determines if should move to next Task and abort this one
         * @return Null means no more task
         * */
        API Nullable Task* nextTask(bool move = false);

        API NotNull Task* nowTask();

        [[nodiscard]] API bool validIndex() const;

        [[nodiscard]] API bool validIndex(unsigned int index) const;

        API bool registerTask(Nullable Task* task);

        API bool isName(const char* compare) const;
    };

    /**
     * @param groupName Null means get the group now working for
     * */
    API Nullable Group* getGroup(const char* groupName = nullptr) noexcept;

    API NotNull Group* nextGroup();

    API bool registerTask(const char* groupName,Task* task,bool create = true);

    API bool registerGroup(Group *group, const char *groupName = nullptr);

    API Nullable Task* nowTask() noexcept(false);

    /**
     * @param move Determines if should move to next Task and abort this one
     * */
    API Nullable Task* nextTask(bool move = false);

    API unsigned int workingIndex();

    API bool validIndex(unsigned int index = workingIndex());

    API void task_from_data(dataStore::Data& data,Task* task);

    API void task_to_data(dataStore::Data& data,const Task* task);

    API void group_from_data(dataStore::Data& data, Group* group);

    API void group_to_data(dataStore::Data& data, const Group* group);
}

}

namespace crawlTask {
    /**
     * For network request which has specific target
     */
    API void GroupFilter(NotNull const string& target);
}