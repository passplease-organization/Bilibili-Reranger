#pragma once

#include <iostream>
#include <map>
#include <sstream>
#include <utility>
#include <nlohmann/json.hpp>
#include "../APIStatus.h"

#define MAX_BUFFER_SIZE 100

using namespace std;
using Json = nlohmann::json;

#define RESET "\033[0m" // 重置颜色
#define RED "\033[31m" // 红色
#define GREEN "\033[32m" // 绿色
#define YELLOW "\033[33m" // 黄色
#define BLUE "\033[34m" // 蓝色

template <class Label,class T>
bool contains(const Label& label,map<const Label,T> map){
    return map.find(label) != map.end();
}

Json getJson(const char* name, const char* path);

#define NONE_THREAD_ID 0
API void setThreadId(const long long& id);

API bool endWith(const string& target,const string& substring);

API bool startWith(const string& target,const string& substring);

API string removeEnd(const string& target,const string& substring);

API string randomString(size_t length);

extern thread_local long long id;
extern thread_local bool logLineStart;

#ifdef __cplusplus
namespace cppUtil {
    struct OutputConfig {
        bool endl = true;
        const char* color = nullptr;
    };

    namespace detail {
        inline void emitThreadPrefixIfNeeded() {
            if (logLineStart && id != NONE_THREAD_ID)
                cout << "[Thread ID: " << id << "] ";
        }

        inline void appendToStream(ostringstream& stream, const char* value) {
            stream << (value == nullptr ? "(null)" : value);
        }

        template <class T>
        inline void appendToStream(ostringstream& stream, T&& value) {
            stream << std::forward<T>(value);
        }

        template <class... Args>
        inline string buildMessage(Args&&... args) {
            ostringstream stream;
            (appendToStream(stream, std::forward<Args>(args)), ...);
            return stream.str();
        }

        inline void emitSay(const char* message, const bool endl, const char* color) {
            emitThreadPrefixIfNeeded();
            if (color != nullptr)
                cout << color;
            cout << (message == nullptr ? "(null)" : message) << RESET;
            logLineStart = endl;
            if (endl)
                cout << '\n';
        }

        [[noreturn]] inline void emitThrowError(const char* error) {
            if (!logLineStart)
                cout << '\n';
            logLineStart = true;
            emitThreadPrefixIfNeeded();
            cout << RED << (error == nullptr ? "(null)" : error) << RESET << '\n';
            logLineStart = true;
            throw runtime_error(error == nullptr ? "(null)" : error);
        }
    }

    template <class... Args>
    inline void say(const OutputConfig& config, Args&&... args) {
        const auto message = detail::buildMessage(std::forward<Args>(args)...);
        detail::emitSay(message.c_str(), config.endl, config.color);
    }

    template <class... Args>
    inline void say(Args&&... args) {
        cppUtil::say({}, std::forward<Args>(args)...);
    }

    template <class... Args>
    inline void warn(const OutputConfig& config, Args&&... args) {
        const auto message = detail::buildMessage(std::forward<Args>(args)...);
        detail::emitSay(message.c_str(), config.endl, config.color == nullptr ? YELLOW : config.color);
    }

    template <class... Args>
    inline void warn(Args&&... args) {
        cppUtil::warn({}, std::forward<Args>(args)...);
    }

    template <class... Args>
    [[noreturn]] inline void throwError(Args&&... args) {
        const auto message = detail::buildMessage(std::forward<Args>(args)...);
        detail::emitThrowError(message.c_str());
    }
}
#endif

extern "C"{

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

API void defaultOutputChar(char** output);

API void freeOutputChar(char** output);

/**
 * @param name just a flag to separate different file in RAM
 * @param path actual file path
 * @param recover recover existing value
 * @return success or not
 */
API bool saveToFile(const char* name,const char* path,bool recover = false);

/**
 * @param name The name of your file mark
 * @param path The path to your file
 * @param release Defined add to map or delete from map, false means adding
 * @return Success(true) or Fail(false)
 * Just store in RAM, if needs written to file, please call `saveToFile`
 * */
API bool storeJson(const char* name,const char* path,const Json& json = nullptr,bool release = false);

}

constexpr bool needCrawlURL(const std::string& url){
    return !url.contains('.');
}

/**
 * A map has a limited elements count
 * @tparam key Must can be copied
 * @tparam value Allow copy constructor deleted
 */
template <typename key,typename value>
class LinkedMap {
protected:
    key* keys;
    value* values;
    unsigned int _size;

public:
    const unsigned int maxCount;
    API LinkedMap(unsigned int count);

    API LinkedMap(const LinkedMap& other) requires std::copy_constructible<value>;
    API LinkedMap(const LinkedMap* other) requires std::copy_constructible<value>;

    API LinkedMap(LinkedMap&& other) noexcept requires std::move_constructible<value>;

    API virtual ~LinkedMap();

    API virtual pair<key,value> put(const key& k,const value& v);
    API virtual pair<key,value> put(const pair<key,value>& p) {
        return put(p.first,p.second);
    }

    API value& operator[](const key& k);

    API bool operator==(const LinkedMap& other) const;

    API [[nodiscard]] Nullable virtual value* get(const key& k) const;

    /**
     * @param k key of element you want to remove, null means the first one
     * @return removed value
     */
    API Nullable virtual value* remove(Nullable const key* k);

    API [[nodiscard]] bool contains(const key& k) const;

    API [[nodiscard]] virtual const unsigned int& size() const {
        return _size;
    }

    API [[nodiscard]] bool empty() const {
        return _size == 0;
    }

    API [[nodiscard]] virtual pair<key,value> last() const {
        return std::pair<key,value>(keys[_size - 1],values[_size - 1]);
    }

    API [[nodiscard]] virtual pair<key,value> first() const {
        return std::pair<key,value>(keys[0],values[0]);
    }
};

namespace webAPI {
    class Client;
}

template class LinkedMap<std::string,webAPI::Client*>;

namespace dataStore{
    using Data = Json;

    API Data readFromJson(const char* path,const char* name = nullptr,bool _throw = false);

    API bool writeToJson(const Data& data,const char* target_name,const char* target_path,bool recover = true,bool storage = true);
}
