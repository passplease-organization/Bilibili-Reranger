#pragma once

#include "../APIStatus.h"

#ifdef __cplusplus
    #include <string>
    #include <toml11/types.hpp>

    using string = std::string;
    using Config = toml::value;
#endif

#define DEFAULT_CONFIG_FILE_TYPE ".toml"

#ifndef MAX_BUFFER_SIZE
    #define MAX_BUFFER_SIZE 100
#endif

extern "C" {
/**
 * Get config file absolute path
 * @param output a variable to get output path
 * @param filePath relative path of you config file, start with your file name and no file type, such as "config/your_file"
 * @param maxLength output data max length
 * @param fileType file type, starts as . for example, .toml .json
 */
API void toConfigPath(char* output, const char* filePath, size_t maxLength = MAX_BUFFER_SIZE, const char *fileType = DEFAULT_CONFIG_FILE_TYPE) noexcept;

/**
 * @param filePath output of toConfigPath
 */
API inline bool deleteConfig(const char* filePath) {
    return remove(filePath) != 0;
}

/**
 * @param filePath output of toConfigPath
 */
API inline bool createConfig(const char* filePath) {
    return fopen(filePath,"a") != nullptr;
}

/**
 *
 * @param output a variable to get output config value( by string )
 * @param maxLength output data max length
 * @param flag the config column you want to get
 * @param path output of toConfigPath
 */
API void getDefaultConfigContent(char* output,size_t maxLength,const char* flag,const char* path);

API bool writeDefaultConfig(const char* filePath,const char* flag,const char* initValue,const char* description = nullptr,bool recover = false);
}

#ifdef __cplusplus
namespace cppUtil {
    /**
     * @param filePath output of toConfigPath
     */
    API Config getConfig(const string& filePath,const bool& clearComment = true);

    API bool saveConfig(const string& filePath,const Config& config);

    template<typename ValueType>
    API void setConfig(Config& config, const string& flag, const ValueType &value, const string& description = "",const bool& recover = false);

    template<typename ValueType>
    concept SupportedValue = requires(Config config,ValueType&& value){
        { config = value } -> std::same_as<Config&>;
    };

    template<typename ValueType>
    concept ConvirtableValue = requires(Config config,const ValueType& value){
        { config = to_toml(config,value) } -> std::same_as<Config&>;
    } && requires(const Config& config,ValueType value){
        { value = from_toml(config,value) } -> std::same_as<ValueType&>;
    };
}
#endif
