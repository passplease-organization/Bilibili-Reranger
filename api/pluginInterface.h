#pragma once

#include "APIStatus.h"
#include "utils/Util.h"

#pragma once

extern "C" {

enum struct PluginStatus {
    FAIL,
    SUCCESS,
    PASS
};

enum struct VideoStatus {
    /**
     * HARD means keep/throw immediately, SOFT means act after all plugin's judgement.
     */
    HARD_KEEP,
    SOFT_KEEP,
    HARD_THROW,
    SOFT_THROW,
    UNKNOWN
};

namespace crawlTask{
    enum struct WorkingMode {
        SEARCH,
        SUBSCRIBE,
        CATEGORY,
        /**
         * Based on one video, use the platform's recommendation gather the videos platform offered, you should offer video's URL
         */
        FIND_MORE,
        /**
         * TAG mode, used on pre-crawl means search by video tags; or used on recommend to client where it should always be
         */
        TAG,
        HOME_PAGE_FILTER,
        /**
         * For plugin to get data they needed, not for browser
         */
        PLUGIN
    };

    API int defaultDaytime(WorkingMode mode);

    API const char* getName(WorkingMode mode);

    API Nullable WorkingMode byName(const char* name);

    struct API Task{
    protected:
        /**
         * Start by 1, 0 means never work
         */
        unsigned int _workCount = -1;
    public:
        const char* keyword;
        WorkingMode mode;
        int videoCount;
        int publishedDay;
        dataStore::Data extraData;// any data the plugin want to write down
        Task(const char* keyword,unsigned int videoCount,WorkingMode mode = WorkingMode::SEARCH,int publishedDay = -1,dataStore::Data extraData = dataStore::Data());
        ~Task();

        bool operator==(const Task &other) const {
            return string(keyword) == string(other.keyword) && mode == other.mode && publishedDay == other.publishedDay;
        }

        Task(const Task &other):keyword(nullptr),mode(other.mode),videoCount(other.videoCount),publishedDay(other.publishedDay),extraData(other.extraData){
            if (other.keyword) {
                const auto temp = new char[std::strlen(other.keyword) + 1];
                std::strcpy(temp, other.keyword);
                keyword = temp;
            }
        }

        API [[nodiscard]] const unsigned int& workCount() const {
            return _workCount;
        }

        API const unsigned int& workOnce() {
            _workCount++;
            return workCount();
        }
    };

    class Group{
    private:
        unsigned int workingIndex = 0;
    public:
        vector<Task*> tasks = vector<Task*>();
        const char* name;
        const char* platform;
        int videoCount;

        API explicit Group(const char* name,const char* platform,unsigned int videoCount = 0,bool regi = false);
        API ~Group();

        API Group* operator+= (Group& other);

        API Group* operator+= (Group* other);

        /**
         * @param move Determines if should move to next Task and abort this one
         * @return Null means no more task
         * */
        API Nullable Task* nextTask(bool move = false);

        [[nodiscard]] API NotNull Task* nowTask() const;

        [[nodiscard]] API bool validIndex() const;

        [[nodiscard]] API bool validIndex(unsigned int index) const;

        API bool registerTask(Nullable Task* task);

        API bool isName(const char* compare) const;
    };

    /**
     * @param groupName Null means get the group now working for
     * */
    API Nullable Group* getGroup(const char* groupName = nullptr,const char* platform = nullptr) noexcept;

    API NotNull Group* nextGroup();

    API bool registerTask(const char* groupName,const char* platform,Task* task,bool create = true);

    API bool registerGroup(Group *group, const char *groupName = nullptr,const char* platform = nullptr);

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
    API void GroupFilter(NotNull const string& target,NotNull const string& platform);

    API const vector<Group*>& getAllGroups();
}

namespace std {
    template <> // To guarteen Task can be used in unordered map
    struct hash<crawlTask::Task> {
        size_t operator()(const crawlTask::Task& task) const noexcept {
            size_t h1 = hash<string>()(task.keyword ? task.keyword : "");
            size_t h2 = hash<int>()(static_cast<int>(task.mode));
            size_t h3 = hash<int>()(task.publishedDay);
            return h1 ^ (h2 << 1) ^ (h3 << 2);
        }
    };
}
