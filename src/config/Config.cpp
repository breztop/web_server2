#include "Config.hpp"
#include <fstream>
#include <iostream>
#include <cstdlib>

namespace bre {

Config& Config::GetInstance() {
    static Config instance;
    return instance;
}

bool Config::LoadFromFile(const std::string& configPath) {
    std::lock_guard<std::mutex> lock(_mutex);
    
    std::ifstream file(configPath);
    if (!file.is_open()) {
        std::cerr << "Failed to open config file: " << configPath << std::endl;
        return false;
    }

    _configMap.clear();
    std::string line;
    int lineNum = 0;

    while (std::getline(file, line)) {
        ++lineNum;
        
        // 移除注释
        auto commentPos = line.find('#');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        // 去除首尾空白
        line = _trim(line);
        
        // 跳过空行
        if (line.empty()) {
            continue;
        }

        // 解析键值对
        auto equalPos = line.find('=');
        if (equalPos == std::string::npos) {
            std::cerr << "Invalid config line " << lineNum << ": " << line << std::endl;
            continue;
        }

        std::string key = _trim(line.substr(0, equalPos));
        std::string value = _trim(line.substr(equalPos + 1));

        if (key.empty()) {
            std::cerr << "Empty key at line " << lineNum << std::endl;
            continue;
        }

        _configMap[key] = value;
    }

    _configPath = configPath;
    std::cout << "Config loaded: " << _configMap.size() << " entries" << std::endl;
    return true;
}

std::optional<std::string> Config::Get(std::string_view key) const {
    std::lock_guard<std::mutex> lock(_mutex);
    
    auto it = _configMap.find(std::string(key));
    if (it != _configMap.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::string Config::GetOrDefault(std::string_view key, std::string_view defaultValue) const {
    auto value = Get(key);
    return value.has_value() ? value.value() : std::string(defaultValue);
}

void Config::LoadFromEnv(const std::string& envPrefix) {
    std::lock_guard<std::mutex> lock(_mutex);
    
    // 常用的环境变量
    std::vector<std::string> envKeys = {
        "PORT", "DB_HOST", "DB_PORT", "DB_NAME", 
        "DB_USER", "DB_PASSWORD", "THREAD_COUNT"
    };
    
    for (const auto& key : envKeys) {
        std::string envName = envPrefix + key;
        const char* env = std::getenv(envName.c_str());
        if (env != nullptr && env[0] != '\0') {
            _configMap[key] = std::string(env);
        }
    }
}

std::string Config::GetSecure(std::string_view key, 
                              const std::string& envKey,
                              std::string_view defaultValue) const {
    // 优先从环境变量读取
    std::string actualEnvKey = envKey.empty() ? ("BRE_" + std::string(key)) : envKey;
    const char* env = std::getenv(actualEnvKey.c_str());
    if (env != nullptr && env[0] != '\0') {
        return std::string(env);
    }
    
    // 其次从配置文件读取
    auto value = Get(key);
    if (value.has_value()) {
        return value.value();
    }
    
    // 最后返回默认值
    return std::string(defaultValue);
}

void Config::Set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(_mutex);
    _configMap[key] = value;
}

bool Config::Has(std::string_view key) const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _configMap.find(std::string(key)) != _configMap.end();
}

std::unordered_map<std::string, std::string> Config::GetAll() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _configMap;
}

void Config::Clear() {
    std::lock_guard<std::mutex> lock(_mutex);
    _configMap.clear();
    _configPath.clear();
}

std::string Config::GetConfigPath() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _configPath;
}

std::string Config::_trim(const std::string& str) {
    const char* whitespace = " \t\n\r\f\v";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return "";
    }
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

} // namespace bre
