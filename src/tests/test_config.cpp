#define BOOST_TEST_MODULE ConfigTest
#include <boost/test/included/unit_test.hpp>
#include "../config/Config.hpp"
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

// 创建测试配置文件
void CreateTestConfigFile(const std::string& filename, const std::string& content) {
    std::ofstream file(filename);
    file << content;
    file.close();
}

// 清理测试文件
struct ConfigTestFixture {
    ~ConfigTestFixture() {
        if (fs::exists("test_config.txt")) {
            fs::remove("test_config.txt");
        }
    }
};

BOOST_FIXTURE_TEST_SUITE(ConfigTestSuite, ConfigTestFixture)

BOOST_AUTO_TEST_CASE(test_load_from_file) {
    CreateTestConfigFile("test_config.txt", 
        "PORT=8080\n"
        "HOST=localhost\n"
        "# This is a comment\n"
        "TIMEOUT=5000\n");

    auto& config = bre::Config::GetInstance();
    BOOST_CHECK(config.LoadFromFile("test_config.txt"));
    
    auto port = config.Get("PORT");
    BOOST_CHECK(port.has_value());
    BOOST_CHECK_EQUAL(port.value(), "8080");
    
    auto host = config.Get("HOST");
    BOOST_CHECK(host.has_value());
    BOOST_CHECK_EQUAL(host.value(), "localhost");
    
    auto timeout = config.Get("TIMEOUT");
    BOOST_CHECK(timeout.has_value());
    BOOST_CHECK_EQUAL(timeout.value(), "5000");
}

BOOST_AUTO_TEST_CASE(test_get_nonexistent_key) {
    auto& config = bre::Config::GetInstance();
    config.Clear();
    
    auto value = config.Get("NONEXISTENT");
    BOOST_CHECK(!value.has_value());
}

BOOST_AUTO_TEST_CASE(test_get_or_default) {
    auto& config = bre::Config::GetInstance();
    config.Clear();
    config.Set("KEY1", "value1");
    
    BOOST_CHECK_EQUAL(config.GetOrDefault("KEY1", "default"), "value1");
    BOOST_CHECK_EQUAL(config.GetOrDefault("KEY2", "default"), "default");
}

BOOST_AUTO_TEST_CASE(test_set_and_has) {
    auto& config = bre::Config::GetInstance();
    config.Clear();
    
    BOOST_CHECK(!config.Has("NEWKEY"));
    config.Set("NEWKEY", "newvalue");
    BOOST_CHECK(config.Has("NEWKEY"));
    
    auto value = config.Get("NEWKEY");
    BOOST_CHECK(value.has_value());
    BOOST_CHECK_EQUAL(value.value(), "newvalue");
}

BOOST_AUTO_TEST_CASE(test_clear) {
    auto& config = bre::Config::GetInstance();
    config.Set("KEY1", "value1");
    config.Set("KEY2", "value2");
    
    BOOST_CHECK(config.Has("KEY1"));
    BOOST_CHECK(config.Has("KEY2"));
    
    config.Clear();
    BOOST_CHECK(!config.Has("KEY1"));
    BOOST_CHECK(!config.Has("KEY2"));
}

BOOST_AUTO_TEST_CASE(test_get_all) {
    auto& config = bre::Config::GetInstance();
    config.Clear();
    config.Set("KEY1", "value1");
    config.Set("KEY2", "value2");
    config.Set("KEY3", "value3");
    
    auto allConfig = config.GetAll();
    BOOST_CHECK_EQUAL(allConfig.size(), 3);
    BOOST_CHECK_EQUAL(allConfig["KEY1"], "value1");
    BOOST_CHECK_EQUAL(allConfig["KEY2"], "value2");
    BOOST_CHECK_EQUAL(allConfig["KEY3"], "value3");
}

BOOST_AUTO_TEST_CASE(test_comments_and_whitespace) {
    CreateTestConfigFile("test_config.txt", 
        "  PORT = 8080  \n"
        "\n"
        "# Comment line\n"
        "HOST=localhost # inline comment\n"
        "  \n"
        "TIMEOUT=5000\n");

    auto& config = bre::Config::GetInstance();
    config.Clear();
    BOOST_CHECK(config.LoadFromFile("test_config.txt"));
    
    BOOST_CHECK_EQUAL(config.GetOrDefault("PORT", ""), "8080");
    BOOST_CHECK_EQUAL(config.GetOrDefault("HOST", ""), "localhost");
    BOOST_CHECK_EQUAL(config.GetOrDefault("TIMEOUT", ""), "5000");
}

BOOST_AUTO_TEST_SUITE_END()
