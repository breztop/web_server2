#define BOOST_TEST_MODULE HttpMethodsTest
#include <boost/test/included/unit_test.hpp>

#include "breutil/buffer.hpp"
#include "http/HttpRequest.hpp"
#include "http/HttpResponse.hpp"

BOOST_AUTO_TEST_SUITE(HttpMethodsTestSuite)

BOOST_AUTO_TEST_CASE(test_parse_put_request) {
    bre::Buffer buffer;
    std::string request =
        "PUT /api/data/1 HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 15\r\n"
        "\r\n"
        "{\"name\":\"test\"}";
    buffer.Append(request);

    bre::HttpRequest httpRequest;
    BOOST_CHECK(httpRequest.Parse(buffer));
    BOOST_CHECK(httpRequest.IsFinished());
    BOOST_CHECK_EQUAL(httpRequest.GetMethodString(), "PUT");
    BOOST_CHECK_EQUAL(httpRequest.GetPath(), "/api/data/1");
    BOOST_CHECK_EQUAL(httpRequest.GetBody(), "{\"name\":\"test\"}");
}

BOOST_AUTO_TEST_CASE(test_parse_delete_request) {
    bre::Buffer buffer;
    std::string request =
        "DELETE /api/users/123 HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Authorization: Bearer token123\r\n"
        "\r\n";
    buffer.Append(request);

    bre::HttpRequest httpRequest;
    BOOST_CHECK(httpRequest.Parse(buffer));
    BOOST_CHECK(httpRequest.IsFinished());
    BOOST_CHECK_EQUAL(httpRequest.GetMethodString(), "delete");
    BOOST_CHECK_EQUAL(httpRequest.GetPath(), "/api/users/123");
    BOOST_CHECK_EQUAL(httpRequest.GetHeader("Authorization"), "Bearer token123");
}

BOOST_AUTO_TEST_CASE(test_parse_patch_request) {
    bre::Buffer buffer;
    std::string body = "{\"email\":\"new@test.com\"}";
    std::string request =
        "PATCH /api/user/1 HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: " +
        std::to_string(body.size()) +
        "\r\n"
        "\r\n" +
        body;
    buffer.Append(request);

    bre::HttpRequest httpRequest;
    BOOST_CHECK(httpRequest.Parse(buffer));
    BOOST_CHECK(httpRequest.IsFinished());
    BOOST_CHECK_EQUAL(httpRequest.GetMethodString(), "PATCH");
    BOOST_CHECK_EQUAL(httpRequest.GetBody(), body);
}

BOOST_AUTO_TEST_CASE(test_parse_options_request) {
    bre::Buffer buffer;
    std::string request =
        "OPTIONS /api/users HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Access-Control-Request-Method: GET,POST,PUT,DELETE\r\n"
        "\r\n";
    buffer.Append(request);

    bre::HttpRequest httpRequest;
    BOOST_CHECK(httpRequest.Parse(buffer));
    BOOST_CHECK(httpRequest.IsFinished());
    BOOST_CHECK_EQUAL(httpRequest.GetMethodString(), "OPTIONS");
    BOOST_CHECK_EQUAL(httpRequest.GetPath(), "/api/users");
}

BOOST_AUTO_TEST_CASE(test_parse_head_request) {
    bre::Buffer buffer;
    std::string request =
        "HEAD /index.html HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
    buffer.Append(request);

    bre::HttpRequest httpRequest;
    BOOST_CHECK(httpRequest.Parse(buffer));
    BOOST_CHECK(httpRequest.IsFinished());
    BOOST_CHECK_EQUAL(httpRequest.GetMethodString(), "HEAD");
    BOOST_CHECK_EQUAL(httpRequest.GetPath(), "/index.html");
}

BOOST_AUTO_TEST_CASE(test_parse_invalid_method) {
    bre::Buffer buffer;
    std::string request =
        "INVALID /test HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
    buffer.Append(request);

    bre::HttpRequest httpRequest;
    BOOST_CHECK(!httpRequest.Parse(buffer));
}

BOOST_AUTO_TEST_CASE(test_parse_path_with_query_string) {
    bre::Buffer buffer;
    std::string request =
        "GET /api/search?q=test&page=1 HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
    buffer.Append(request);

    bre::HttpRequest httpRequest;
    BOOST_CHECK(httpRequest.Parse(buffer));
    BOOST_CHECK_EQUAL(httpRequest.GetPath(), "/api/search?q=test&page=1");
}

BOOST_AUTO_TEST_CASE(test_parse_path_with_fragment) {
    bre::Buffer buffer;
    std::string request =
        "GET /docs#section1 HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
    buffer.Append(request);

    bre::HttpRequest httpRequest;
    BOOST_CHECK(httpRequest.Parse(buffer));
    BOOST_CHECK_EQUAL(httpRequest.GetPath(), "/docs#section1");
}

BOOST_AUTO_TEST_CASE(test_header_value_with_colon) {
    bre::Buffer buffer;
    std::string request =
        "GET /test HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Authorization: Bearer token:with:colons\r\n"
        "\r\n";
    buffer.Append(request);

    bre::HttpRequest httpRequest;
    BOOST_CHECK(httpRequest.Parse(buffer));
    BOOST_CHECK_EQUAL(httpRequest.GetHeader("Authorization"), "Bearer token:with:colons");
}

BOOST_AUTO_TEST_CASE(test_header_value_with_spaces) {
    bre::Buffer buffer;
    std::string request =
        "GET /test HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "X-Custom:   value with spaces   \r\n"
        "\r\n";
    buffer.Append(request);

    bre::HttpRequest httpRequest;
    BOOST_CHECK(httpRequest.Parse(buffer));
    BOOST_CHECK_EQUAL(httpRequest.GetHeader("X-Custom"), "value with spaces   ");
}

BOOST_AUTO_TEST_CASE(test_has_header) {
    bre::Buffer buffer;
    std::string request =
        "GET /test HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Type: application/json\r\n"
        "\r\n";
    buffer.Append(request);

    bre::HttpRequest httpRequest;
    BOOST_CHECK(httpRequest.Parse(buffer));
    BOOST_CHECK(httpRequest.HasHeader("Host"));
    BOOST_CHECK(httpRequest.HasHeader("Content-Type"));
    BOOST_CHECK(!httpRequest.HasHeader("X-Not-Exist"));
}

BOOST_AUTO_TEST_CASE(test_missing_header_value) {
    bre::Buffer buffer;
    std::string request =
        "GET /test HTTP/1.1\r\n"
        "Host:\r\n"
        "\r\n";
    buffer.Append(request);

    bre::HttpRequest httpRequest;
    BOOST_CHECK(httpRequest.Parse(buffer));
    BOOST_CHECK_EQUAL(httpRequest.GetHeader("Host"), "");
}

BOOST_AUTO_TEST_CASE(test_empty_body) {
    bre::Buffer buffer;
    std::string request =
        "POST /api/data HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 0\r\n"
        "\r\n";
    buffer.Append(request);

    bre::HttpRequest httpRequest;
    BOOST_CHECK(httpRequest.Parse(buffer));
    BOOST_CHECK(httpRequest.IsFinished());
    BOOST_CHECK_EQUAL(httpRequest.GetBody(), "");
}

BOOST_AUTO_TEST_CASE(test_large_body) {
    bre::Buffer buffer;
    std::string largeBody(1000, 'A');
    std::string request =
        "POST /api/data HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: " +
        std::to_string(largeBody.size()) +
        "\r\n"
        "\r\n" +
        largeBody;
    buffer.Append(request);

    bre::HttpRequest httpRequest;
    BOOST_CHECK(httpRequest.Parse(buffer));
    BOOST_CHECK(httpRequest.IsFinished());
    BOOST_CHECK_EQUAL(httpRequest.GetBody().size(), 1000);
}

BOOST_AUTO_TEST_CASE(test_incomplete_body) {
    bre::Buffer buffer;
    std::string request =
        "POST /api/data HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 100\r\n"
        "\r\n"
        "partial";
    buffer.Append(request);

    bre::HttpRequest httpRequest;
    BOOST_CHECK(!httpRequest.Parse(buffer));
    BOOST_CHECK(!httpRequest.IsFinished());
}

BOOST_AUTO_TEST_CASE(test_double_colon_in_header) {
    bre::Buffer buffer;
    std::string request =
        "GET /test HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "X-Header: value:with:colons\r\n"
        "\r\n";
    buffer.Append(request);

    bre::HttpRequest httpRequest;
    BOOST_CHECK(httpRequest.Parse(buffer));
    BOOST_CHECK_EQUAL(httpRequest.GetHeader("X-Header"), "value:with:colons");
}

BOOST_AUTO_TEST_SUITE_END()
