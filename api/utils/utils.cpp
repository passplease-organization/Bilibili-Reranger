#include <filesystem>
#include <iostream>
#include <fstream>
#include <vector>
#include <random>

#include "configUtil.h"
#include "Event.h"
#include "../develop/flags.h"
#include "Util.h"

using namespace std;
using Json = nlohmann::json;
namespace fs = std::filesystem;

const string ConfigPath = "config";
auto storedJson = map<string,Json>();

thread_local long long id = NONE_THREAD_ID;
thread_local bool logLineStart = true;

void setThreadId(const long long& _id) {
    if (id == NONE_THREAD_ID)
        id = _id;
}

bool endWith(const char* target,const char* substring){
    auto t = string(target);
    auto s = string(substring);
    if(t.size() < s.size())
        return false;
    return t.substr(t.size() - s.size()) == s;
}

bool endWith(const string& target,const string& substring) {
    if (target.size() < substring.size())
        return false;
    return target.substr(target.size() - substring.size()) == substring;
}

bool startWith(const char* target,const char* substring){
    string t = string(target);
    string s = string(substring);
    if(t.size() < s.size())
        return false;
    return t.substr(0, s.size()) == s;
}

bool startWith(const string& target,const string& substring) {
    if(target.size() < substring.size())
        return false;
    return target.substr(0, substring.size()) == substring;
}

string removeEnd(const string& target,const string& substring){
    return endWith(target.c_str(),substring.c_str()) ? target.substr(0,target.size() - substring.size()) : target;
}

string randomString(size_t length){
    static const char kAlphabet[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~";
    if (length == 0)
        return "";
    thread_local mt19937_64 rng{random_device{}()};
    uniform_int_distribution<size_t> dist(0, sizeof(kAlphabet) - 2);
    string out;
    out.reserve(length);
    for (size_t i = 0; i < length; ++i)
        out.push_back(kAlphabet[dist(rng)]);
    return out;
}

int removeEnd(const char* target,const char* substring,char** buffer){
    string t = string(target);
    string s = string(substring);
    string back = removeEnd(t,s);
    defaultOutputChar(buffer);
    return snprintf(*buffer,MAX_BUFFER_SIZE,"%s",back.c_str());
}

bool fileExists(const char* name, bool absolute){
    return name != nullptr && (absolute ? fs::exists(fs::absolute(name)) : fs::exists(name));
}

void throwError(const char* error) noexcept(false){
    cppUtil::detail::emitThrowError(error);
}

void say(const char* message,bool endl,const char* color){
    cppUtil::detail::emitSay(message, endl, color);
}

void warn(const char* warn,bool endl){
    say(warn,endl,YELLOW);
}

Json getJson(const char* name, const char* path){
    if(fileExists(path)){
        Json json;
        ifstream file(path);
        file >> json;
        return json;
    }
    if(name != nullptr){
        auto json = storedJson.find(name);
        if(json != storedJson.end())
            return json -> second;
    }
    return nullptr;
}

void defaultOutputChar(char** output){
    *output = new char[MAX_BUFFER_SIZE];
}

void freeOutputChar(char** output){
    delete[] *output;
}

bool saveToFile(const char* name,const char* path,bool recover){
    auto json = getJson(name,nullptr);
    if(json == nullptr)
        json = getJson(name,path);
    if(json != nullptr){
        const auto& dir = fs::path(path).parent_path();
        if(!dir.empty() && !fs::exists(dir)){
            fs::create_directories(dir);
        }
        ofstream file(path,ios::out);
        if(recover)
            file.clear();
        file << json;
        file.close();
        return true;
    }else return false;
}

bool storeJson(const char* name,const char* path,const Json& json,bool release){
    if(release){
        storedJson.erase(name);
        return !storedJson.contains(name);
    }else {
        storedJson[name] = json == nullptr ? getJson(name,path) : json;
        return storedJson.contains(name);
    }
}

namespace dataStore{

    Data readFromJson(const char *path, const char *name, const bool _throw) {
        ifstream file;
        char* filePath = nullptr;

        if(fileExists(path)){
            file = ifstream(path);
        }else {
            defaultOutputChar(&filePath);
            toConfigPath(filePath,path,MAX_BUFFER_SIZE,".json");
            if(!fileExists(filePath)) {
                if(_throw) {
                    string error("Path not exists ! Current path: ");
                    error.append(path);
                    cppUtil::throwError(error);
                }
                freeOutputChar(&filePath);
                return nullptr;
            }
            file = ifstream(filePath);
        }

        if(!file.is_open()) {
            if(filePath != nullptr)
                freeOutputChar(&filePath);
            return nullptr;
        }

        try {
            Json json;
            if (std::filesystem::is_empty(filePath))// empty file
                json = Json::object();
            else file >> json;
            file.close();
            if(name != nullptr) {
                const char* storedPath = filePath == nullptr ? path : filePath;
                storeJson(name,storedPath,json);
            }
            if(filePath != nullptr)
                freeOutputChar(&filePath);
            return json;
        }catch(const std::exception& e) {
            if(filePath != nullptr)
                freeOutputChar(&filePath);
            cppUtil::say({false, RED}, "Invalid Json File !");
            cppUtil::say({false, BLUE}, "Exception Info: ");
            cppUtil::say({true, BLUE}, e.what());
            if(_throw)
                cppUtil::throwError(e.what());
            return nullptr;
        }
    }

    bool writeToJson(const Data& data,const char *target_name, const char *target_path,bool recover, bool storage) {
        Json json = recover ? Json::object() : getJson(target_name, target_path);
        if (!recover && json == nullptr) {
            string error = "File not exists ! Cannot write Json ! File path: ";
            error.append(target_path);
            cppUtil::throwError(error);
        }

        if (json.is_object() && data.is_object())
            json.update(data);
        else
            json = data;

        storeJson(target_name,target_path,json);
        return !storage || saveToFile(target_name, target_path);
    }
}

    template<typename key, typename value>
LinkedMap<key, value>::LinkedMap(unsigned int count) : maxCount(count) {
    keys = new key[maxCount];
    values = new value[maxCount];
    _size = 0;
}

template<typename key, typename value>
LinkedMap<key, value>::~LinkedMap() {
    if (keys != nullptr)
        delete[] keys;
    if (values != nullptr)
        delete[] values;
}

template<typename key, typename value>
LinkedMap<key, value>::LinkedMap(const LinkedMap &other) requires std::copy_constructible<value>
: maxCount(other.maxCount) {
    keys = new key[maxCount];
    values = new value[maxCount];

    for (unsigned int i = 0; i < other.size(); i++) {
        keys[i] = other.keys[i];
        values[i] = other.values[i];
    }
    _size = other._size;
}

template<typename key, typename value>
LinkedMap<key, value>::LinkedMap(const LinkedMap *other) requires std::copy_constructible<value>
: maxCount(other->maxCount) {
    keys = new key[maxCount];
    values = new value[maxCount];

    for (unsigned int i = 0; i < other -> size(); i++) {
        keys[i] = other -> keys[i];
        values[i] = other -> values[i];
    }
    _size = other -> _size;
}

template<typename key, typename value>
LinkedMap<key, value>::LinkedMap(LinkedMap &&other) noexcept requires std::move_constructible<value>
: maxCount(other.maxCount) {
    keys = other.keys;
    values = other.values;

    other.keys = nullptr;
    other.values = nullptr;

    _size = other._size;
}

template<typename key, typename value>
bool LinkedMap<key, value>::contains(const key &k) const {
    for (unsigned int i = 0; i < size(); i++) {
        if (keys[i] == k)
            return true;
    }
    return false;
}

template<typename key, typename value>
value &LinkedMap<key, value>::operator[](const key &k) {
    for (unsigned int i = 0; i < size(); i++) {
        if (keys[i] == k)
            return values[i];
    }
    put(k, value{});
    return values[_size - 1];
}

template<typename key, typename value>
bool LinkedMap<key, value>::operator==(const LinkedMap& other) const {
    if (maxCount != other.maxCount || _size != other._size)
        return false;
    for (unsigned int i = 0; i < size(); i++)
        if (keys[i] != other.keys[i] || values[i] != other.values[i])
            return false;
    return true;
}

template<typename key, typename value>
value* LinkedMap<key, value>::get(const key &k) const {
    for (unsigned int i = 0; i < size(); i++) {
        if (keys[i] == k)
            return &values[i];
    }
    return nullptr;
}

template<typename key, typename value>
pair<key, value> LinkedMap<key, value>::put(const key &k, const value &v) {
    for (unsigned int i = 0; i < size(); i++) {
        if (keys[i] == k) {
            auto& old = values[i];
            values[i] = v;
            return pair<key, value>(k,old);
        }
    }
    if (_size < maxCount) {
        keys[_size] = k;
        values[_size] = v;
        _size++;
        return pair<key, value>();
    }
    auto pair = first();
    for (unsigned int i = 0; i < maxCount - 1; i++) {
        keys[i] = keys[i + 1];
        values[i] = values[i + 1];
    }
    keys[maxCount - 1] = k;
    values[maxCount - 1] = v;
    return pair;
}

template<typename key, typename value>
value *LinkedMap<key, value>::remove(const key *k) {
    for (unsigned int i = 0; i < size(); i++) {
        if (k == nullptr || keys[i] == *k) {
            for (unsigned int j = i; j < size() - 1; j++) {
                keys[j] = keys[j + 1];
                values[j] = values[j + 1];
                _size--;
            }
        }
    }
    return nullptr;
}

void(FUNCTION_CALLER* eventHandler)(Event::Event* const&) = nullptr;
void Event::setExporter(void (FUNCTION_CALLER* exporter)(Event *const &)) {
    if (eventHandler == nullptr) {
        eventHandler = exporter;
        cppUtil::say(BLUE,"事件处理器已设置");
    }
}

inline thread_local bool exporting = false;
void Event::exportEvent(Event *const &event) {
#if MORE_DETAILS
    cppUtil::say(BLUE,"广播事件：",event -> description());
#endif
    if (eventHandler == nullptr)
        cppUtil::throwError("错误事件触发，事件广播器还未设置，事件：",event -> description());
    if (exporting)
        cppUtil::throwError("不正确事件广播，正在处理上一个事件！");
    exporting = true;
    eventHandler(event);
    exporting = false;
#if MORE_DETAILS
    cppUtil::say(BLUE,"广播完成！");
#endif
}
