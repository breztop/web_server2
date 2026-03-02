#ifndef BRE_CONFIG_HPP
#define BRE_CONFIG_HPP

#include <string>
#include <unordered_map>
#include <optional>
#include <mutex>
#include <string_view>

namespace bre {

/**
 * @brief 配置管理类 - 单例模式
 * 支持从配置文件读取键值对配置
 * 线程安全的配置访问
 */
class Config {
public:
    // 获取单例实例
    static Config& GetInstance();

    // 禁止拷贝和赋值
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    /**
     * @brief 从配置文件加载配置
     * @param configPath 配置文件路径
     * @return 成功返回true，失败返回false
     */
    bool LoadFromFile(const std::string& configPath);

    /**
     * @brief 从环境变量加载配置
     * @param envPrefix 环境变量前缀 (默认为 "BRE_")
     */
    void LoadFromEnv(const std::string& envPrefix = "BRE_");

    /**
     * @brief 获取配置值
     * @param key 配置键
     * @return 如果存在返回配置值，否则返回空
     */
    std::optional<std::string> Get(std::string_view key) const;

    /**
     * @brief 获取安全配置值(优先从环境变量读取)
     * @param key 配置键
     * @param envKey 环境变量键(为空时使用 "BRE_" + key)
     * @param defaultValue 默认值
     * @return 配置值或默认值
     */
    std::string GetSecure(std::string_view key, 
                         const std::string& envKey = "",
                         std::string_view defaultValue = "") const;

    /**
     * @brief 获取配置值，如果不存在则返回默认值
     * @param key 配置键
     * @param defaultValue 默认值
     * @return 配置值或默认值
     */
    std::string GetOrDefault(std::string_view key, std::string_view defaultValue) const;

    /**
     * @brief 设置配置值
     * @param key 配置键
     * @param value 配置值
     */
    void Set(const std::string& key, const std::string& value);

    /**
     * @brief 检查配置键是否存在
     * @param key 配置键
     * @return 存在返回true，否则返回false
     */
    bool Has(std::string_view key) const;

    /**
     * @brief 获取所有配置项
     * @return 配置映射表的拷贝
     */
    std::unordered_map<std::string, std::string> GetAll() const;

    /**
     * @brief 清空所有配置
     */
    void Clear();

    /**
     * @brief 获取配置文件路径
     * @return 配置文件路径
     */
    std::string GetConfigPath() const;

private:
    Config() = default;
    ~Config() = default;

    // 去除字符串首尾空白
    static std::string _trim(const std::string& str);

    mutable std::mutex _mutex;                              // 互斥锁
    std::unordered_map<std::string, std::string> _configMap; // 配置映射表
    std::string _configPath;                                 // 配置文件路径
};

} // namespace bre

#endif // BRE_CONFIG_HPP
