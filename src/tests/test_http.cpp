#define BOOST_TEST_MODULE HttpTest
#include <boost/test/included/unit_test.hpp>
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"
#include "../buffer/Buffer.hpp"
#include <filesystem>
#include <fstream>

BOOST_AUTO_TEST_SUITE(HttpTestSuite)

// HttpRequest Tests
BOOST_AUTO_TEST_CASE(test_parse_get_request) {
    bre::Buffer buffer;
    std::string request = "GET /index.html HTTP/1.1\r\n"
                         "Host: localhost\r\n"
                         "Connection: keep-alive\r\n"
                         "\r\n";
    buffer.Append(request);

    bre::HttpRequest httpRequest;
    BOOST_CHECK(httpRequest.Parse(buffer));
    BOOST_CHECK(httpRequest.IsFinished());
    BOOST_CHECK_EQUAL(httpRequest.GetMethod(), bre::HttpMethod::GET);
    BOOST_CHECK_EQUAL(httpRequest.GetPath(), "/index.html");
    BOOST_CHECK_EQUAL(httpRequest.GetVersion(), "HTTP/1.1");
    BOOST_CHECK_EQUAL(httpRequest.GetHeader("Host"), "localhost");
    BOOST_CHECK_EQUAL(httpRequest.GetHeader("Connection"), "keep-alive");
}

BOOST_AUTO_TEST_CASE(test_parse_post_request_with_body) {
    bre::Buffer buffer;
    std::string request = "POST /api/data HTTP/1.1\r\n"
                         "Host: localhost\r\n"
                         "Content-Type: application/json\r\n"
                         "Content-Length: 13\r\n"
                         "\r\n"
                         "{\"key\":\"val\"}";
    buffer.Append(request);

    bre::HttpRequest httpRequest;
    BOOST_CHECK(httpRequest.Parse(buffer));
    BOOST_CHECK(httpRequest.IsFinished());
    BOOST_CHECK_EQUAL(httpRequest.GetMethod(), bre::HttpMethod::POST);
    BOOST_CHECK_EQUAL(httpRequest.GetPath(), "/api/data");
    BOOST_CHECK_EQUAL(httpRequest.GetBody(), "{\"key\":\"val\"}");
}

BOOST_AUTO_TEST_CASE(test_parse_incomplete_request) {
    bre::Buffer buffer;
    std::string request = "GET /index.html HTTP/1.1\r\n"
                         "Host: localhost\r\n";
    buffer.Append(request);

    bre::HttpRequest httpRequest;
    BOOST_CHECK(!httpRequest.Parse(buffer));  // 不完整
    BOOST_CHECK(!httpRequest.IsFinished());

    // 添加剩余部分
    buffer.Append("Connection: close\r\n\r\n");
    BOOST_CHECK(httpRequest.Parse(buffer));
    BOOST_CHECK(httpRequest.IsFinished());
}

BOOST_AUTO_TEST_CASE(test_parse_multiple_headers) {
    bre::Buffer buffer;
    std::string request = "GET /test HTTP/1.1\r\n"
                         "Host: localhost\r\n"
                         "User-Agent: TestAgent/1.0\r\n"
                         "Accept: text/html\r\n"
                         "Accept-Encoding: gzip\r\n"
                         "\r\n";
    buffer.Append(request);

    bre::HttpRequest httpRequest;
    BOOST_CHECK(httpRequest.Parse(buffer));
    BOOST_CHECK_EQUAL(httpRequest.GetHeader("Host"), "localhost");
    BOOST_CHECK_EQUAL(httpRequest.GetHeader("User-Agent"), "TestAgent/1.0");
    BOOST_CHECK_EQUAL(httpRequest.GetHeader("Accept"), "text/html");
    BOOST_CHECK_EQUAL(httpRequest.GetHeader("Accept-Encoding"), "gzip");
}

BOOST_AUTO_TEST_CASE(test_request_reset) {
    bre::Buffer buffer;
    buffer.Append("GET /test HTTP/1.1\r\nHost: localhost\r\n\r\n");

    bre::HttpRequest httpRequest;
    BOOST_CHECK(httpRequest.Parse(buffer));
    BOOST_CHECK(httpRequest.IsFinished());

    httpRequest.Reset();
    BOOST_CHECK(!httpRequest.IsFinished());
    BOOST_CHECK_EQUAL(httpRequest.GetPath(), "");
}

// HttpResponse Tests
BOOST_AUTO_TEST_CASE(test_build_simple_response) {
    bre::HttpResponse response;
    response.SetStatus(bre::HttpStatus::OK);
    response.SetBody("Hello, World!");
    response.SetContentType("text/plain");

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("HTTP/1.1 200 OK") != std::string::npos);
    BOOST_CHECK(result.find("Content-Type: text/plain") != std::string::npos);
    BOOST_CHECK(result.find("Content-Length: 13") != std::string::npos);
    BOOST_CHECK(result.find("Hello, World!") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_error_response) {
    auto response = bre::HttpResponse::MakeErrorResponse(
        bre::HttpStatus::NOT_FOUND, "Page not found");

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("404 Not Found") != std::string::npos);
    BOOST_CHECK(result.find("Page not found") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_json_response) {
    auto response = bre::HttpResponse::MakeJsonResponse(R"({"status":"ok"})");

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("200 OK") != std::string::npos);
    BOOST_CHECK(result.find("application/json") != std::string::npos);
    BOOST_CHECK(result.find(R"({"status":"ok"})") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_text_response) {
    auto response = bre::HttpResponse::MakeTextResponse("Plain text");

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("text/plain") != std::string::npos);
    BOOST_CHECK(result.find("Plain text") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_keep_alive_header) {
    bre::HttpResponse response;
    response.SetStatus(bre::HttpStatus::OK);
    response.SetKeepAlive(true);
    response.SetBody("Test");

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("Connection: keep-alive") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_custom_headers) {
    bre::HttpResponse response;
    response.SetStatus(bre::HttpStatus::OK);
    response.AddHeader("X-Custom-Header", "CustomValue");
    response.AddHeader("X-Another-Header", "AnotherValue");
    response.SetBody("Test");

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("X-Custom-Header: CustomValue") != std::string::npos);
    BOOST_CHECK(result.find("X-Another-Header: AnotherValue") != std::string::npos);
}

// 文件测试需要创建临时文件
struct FileTestFixture {
    std::string testFile = "test_temp.html";
    
    FileTestFixture() {
        std::ofstream file(testFile);
        file << "<html><body>Test</body></html>";
        file.close();
    }
    
    ~FileTestFixture() {
        std::filesystem::remove(testFile);
    }
};

BOOST_FIXTURE_TEST_CASE(test_load_file, FileTestFixture) {
    bre::HttpResponse response;
    BOOST_CHECK(response.LoadFile(testFile));

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("<html><body>Test</body></html>") != std::string::npos);
    BOOST_CHECK(result.find("text/html") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_load_nonexistent_file) {
    bre::HttpResponse response;
    BOOST_CHECK(!response.LoadFile("nonexistent_file.txt"));
}

BOOST_AUTO_TEST_SUITE_END()
