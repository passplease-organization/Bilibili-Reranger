#include "configUtil.h"

#include <filesystem>
#include <toml.hpp>
#include <fstream>

constexpr string ConfigPath = "config";

void toConfigPath(char* output, const char* filePath, size_t maxLength, const char *fileType) noexcept {
    string path{};
#ifdef WIN32
    path = path.append(ConfigPath).append("\\").append(filePath).append(fileType);
#elifdef __linux__
    path = path.append(ConfigPath).append("/").append(filePath).append(fileType);
#endif
    snprintf(output,maxLength,"%s",path.c_str());
}

Config cppUtil::getConfig(const string& filePath,const bool& clearComment) {
    auto&& c = toml::parse(filePath);
    if (clearComment)
        c.comments().clear();
    return c;
}

bool cppUtil::saveConfig(const string &filePath, const Config &config) {
    try {
        std::ofstream file(filePath, std::ios::out);
        return file.is_open() && (file << config, file.good());
    }catch (...) {
        return false;
    }
}

void getDefaultConfigContent(char *output, size_t maxLength, const char *flag, const char *path) {
    auto config = cppUtil::getConfig(path)[flag];
    string back;
    if (config.is_string())
        back = config.as_string();
    else back = format(config);
    snprintf(output,maxLength,"%s",back.c_str());
}

bool writeConfig(const char *filePath, const char *flag, const char *initValue, const char *description) {
    auto config = cppUtil::getConfig(filePath);
    auto& value = config[flag];
    value = initValue;
    if (description != nullptr && description[0] != '\0')
        value.comments().push_back(description);
    return cppUtil::saveConfig(filePath, config);
}

template<typename ValueType>
void cppUtil::setConfig(Config &config, const string &flag, const ValueType &value, const string &description,const bool& recover) {
    static_assert(
         cppUtil::SupportedValue<ValueType> || cppUtil::ConvirtableValue<ValueType>,
        "Unsupported value type"
        );

    if (config.contains(flag) && !recover)
        return;
    auto& v = config[flag];
    if constexpr (SupportedValue<ValueType>)
        v = value;
    else v = to_toml(value);
    if (!description.empty())
        v.comments().push_back(description);
}

#define setConfig_declare(type) template void cppUtil::setConfig<type>(Config&, const string&, const type&, const string&, const bool&)

setConfig_declare(int);
setConfig_declare(bool);
setConfig_declare(string);
setConfig_declare(float);
setConfig_declare(double);