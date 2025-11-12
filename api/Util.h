#pragma once

#include <map>
#include <nlohmann/json.hpp>
#include "APIStatus.h"

#define MAX_BUFFER_SIZE 100

using namespace std;
using Json = nlohmann::json;

template <class Label,class T>
bool contains(const Label& label,map<const Label,T> map){
    return map.find(label) != map.end();
}

Json getJson(const char* name, const char* path);

extern "C"{

#define RESET "\033[0m" // 重置颜色
#define RED "\033[31m" // 红色
#define GREEN "\033[32m" // 绿色
#define YELLOW "\033[33m" // 黄色
#define BLUE "\033[34m" // 蓝色

API bool endWith(const char* target,const char* substring);

API bool startWith(const char* target,const char* substring);

/**
 * @param buffer DLL initialize, need call function: <p style="color:red">freeOutputChar</p> to free memory
 * */
API int removeEnd(const char* target,const char* substring,char** buffer);

API bool fileExists(const char* name, bool absolute = false);

API void throwError(const char* error) noexcept(false);

API void say(const char* message,bool endl = true,const char* color = nullptr);

API void warn(const char* warning,bool endl = true);

API bool convertToInt(const char* str, int& num);

API void defaultOutputChar(char** output);

API void freeOutputChar(char** output);

API void deleteConfig(const char* filePath,bool absolute = false, const char* fileType = ".json");

/**
 * Create a config file (default .Json)
 * */
API bool createConfig(Nullable char* output, const char* filePath,const size_t maxLength = MAX_BUFFER_SIZE, const char* fileType = ".json");

/**
 * Get config file path, other function's parameter: path will call automatically
 * */
API bool getConfig(char* output, const char* filePath,const size_t maxLength = MAX_BUFFER_SIZE, const char* fileType = ".json");

API void toConfigPath(char* back, const char* filePath,const size_t maxLength = MAX_BUFFER_SIZE, const char* fileType = ".json");

API bool saveToFile(const char* name,const char* path,bool recover = false);

/**
 * @deprecated
 * @param name The name of your file mark
 * @param path The path to your file
 * @param release Defined add to map or delete from map, false means adding
 * @return Success(true) or Fail(false)
 * */
API bool storeJson(const char* name,const char* path,const Json& json = nullptr,bool release = false);

namespace dataStore{
    class Data;

    API void from_json(const Json& json,Data& data);

    API void to_json(Json& json,const Data& data);

    class Data {
        friend void setSaved(Data *data, bool saved);
    private:
        bool _valid = false;

        bool saved = true;
        bool _neverSave = false;
    public:
        map<const string,Data> data = map<const string,Data>();// TODO 动态初始化
        const static string DATA;
        map<const string,vector<Data>> dataArrays = map<const string,vector<Data>>();
        const static string DATA_ARRAY;

        map<const string,string> strings = map<const string,string>();
        const static string STRING;
        map<const string,vector<string>> stringArrays = map<const string,vector<string>>();
        const static string STRING_ARRAY;

        map<const string,int> ints = map<const string,int>();
        const static string INT;
        map<const string,vector<int>> intArrays = map<const string,vector<int>>();
        const static string INT_ARRAY;

        map<const string,float> floats = map<const string,float>();
        const static string FLOAT;
        map<const string,vector<float>> floatArrays = map<const string,vector<float>>();
        const static string FLOAT_ARRAY;

        map<const string,bool> bools = map<const string,bool>();
        const static string BOOL;
        map<const string,vector<bool>> boolArrays = map<const string,vector<bool>>();
        const static string BOOL_ARRAY;

        string name;

        string path;

        API explicit Data(bool valid = true);

        API Data(const Data& other);

        API explicit Data(const Data* other);

        API Data(Data&& old) noexcept;

        API ~Data();

        API void clear();

        [[nodiscard]] API bool valid() const;

        API void broken();

        API void setName(const char* _name, bool force = false);

        /**
         * @param _path Return value of config()
         * */
        API void setPath(const char* _path, bool force = false);

        API Data& operator=(const Data* other);

        API Data& operator=(const Data& other);

        API bool operator==(const Data& other) const;

        API bool operator!=(const Data& other) const;

        API Data * operator+=(const dataStore::Data *other);

        API Data* operator+=(const Data& other);

        API Data operator+(const Data* other) const;

        API void copy(Data** a) const;

        [[nodiscard]] API bool needSave() const;

        [[nodiscard]] API bool neverSave() const;

        API void NeverSave();

        /**
         * Check if the input is contained in this
         * */
        API void validData(const Data* input) noexcept(false);

        /**
         * @param recover false means merge new Data to old Data, true means just recover old value
         * @param vector put the value to vector or not, if true, the recover is false all the time
         * @param content could be nullptr, if that, means remove the value
         *
         * Parameters are the same for followings
         * */
        API void put(const char *label, const dataStore::Data *content, bool vector = false, bool recover = true);

        API void put(const char *label, const char *content, bool vector = false, bool recover = true);

        API void put(const char *label, const int *content, bool vector = false, bool recover = true);

        API void put(const char *label, const float *content, bool vector = false, bool recover = true);

        API void put(const char *label, const bool *content, bool vector = false, bool recover = true);

        API void put(const char *label, const dataStore::Data &content, bool vector = false, bool recover = true);

        API void put(const char *label, const int &content, bool vector = false, bool recover = true);

        API void put(const char *label, const float &content, bool vector = false, bool recover = true);

        API void put(const char *label, const bool &content, bool vector = false, bool recover = true);

        /**
         * All get function won't recurse to find label, just find in this object.
         * */
        API Nullable void get(const char* label,dataStore::Data** data,bool copy = false);
        API Nullable void get(const char* label,vector<dataStore::Data>** data,bool copy = false);

        /**
         * @param copy For string, this parameter is invalid, only true is allowed
         * */
        API Nullable void get(const char* label,const char** string,bool copy = true) const;
        API Nullable void get(const char* label,NotNull vector<const char*>** string,bool copy = false) const;

        API Nullable void get(const char* label,int** ints,bool copy = false);
        API Nullable void get(const char* label,vector<int>** ints,bool copy = false);

        API Nullable void get(const char* label,float** floats,bool copy = false);
        API Nullable void get(const char* label,vector<float>** floats,bool copy = false);

        API Nullable void get(const char* label,bool** bools,bool copy = false);
        API Nullable void get(const char* label,vector<bool>** bools,bool copy = false);

        API bool empty();

        /**
         * Save to Json, also to file
         * @param path The path to your file (path in computer or key in map)
         * @param data Data you want to write
         * @param storage Store to map or release, true means store, false means delete
         * */
        API bool writeToJson(const char* target_name,const char* target_path,bool recover = true,bool storage = true);

        API bool writeToJson();

        API static Data readFromJson(const char* path,const char* name = nullptr,bool _throw = false);
    };
}

}

namespace dataStore{
    template<typename T>
    void put(Data* data,const char* label,const T *content, bool vector = false, bool recover = true);

    template<typename T>
    void put(Data* data,const char* label,NotNull const vector<T> *content);

    template<typename T>
    void getMap(Data* data,const char *label,T** input,bool copy = false);

    template<typename T>
    void getVector(Data* data,const char *label,vector<T>** input,bool copy = false);

    template<typename T>
    map<const string,T>* getMap(Data* data);

    template<typename T>
    map<const string,vector<T>>* getVector(Data* data);
}
