#define BOOST_TEST_MODULE HttpStatusTest
#include <boost/test/included/unit_test.hpp>

#include "breutil/buffer.hpp"
#include "http/HttpResponse.hpp"

BOOST_AUTO_TEST_SUITE(HttpStatusTestSuite)

BOOST_AUTO_TEST_CASE(test_status_200) {
    bre::HttpResponse response;
    response.SetStatus(bre::HttpStatus::OK);
    response.SetBody("OK");

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("HTTP/1.1 200 OK") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_status_201) {
    bre::HttpResponse response;
    response.SetStatus(bre::HttpStatus::CREATED);
    response.SetBody("Created");

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("HTTP/1.1 201 Created") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_status_204) {
    bre::HttpResponse response;
    response.SetStatus(bre::HttpStatus::NO_CONTENT);

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("HTTP/1.1 204 No Content") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_status_301) {
    auto response = bre::HttpResponse::MakeErrorResponse(bre::HttpStatus::MOVED_PERMANENTLY);

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("301 Moved Permanently") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_status_302) {
    auto response = bre::HttpResponse::MakeErrorResponse(bre::HttpStatus::FOUND);

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("302 Found") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_status_400) {
    auto response =
        bre::HttpResponse::MakeErrorResponse(bre::HttpStatus::BAD_REQUEST, "Invalid request");

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("400 Bad Request") != std::string::npos);
    BOOST_CHECK(result.find("Invalid request") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_status_401) {
    auto response = bre::HttpResponse::MakeErrorResponse(bre::HttpStatus::UNAUTHORIZED,
                                                         "Authentication required");

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("401 Unauthorized") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_status_403) {
    auto response =
        bre::HttpResponse::MakeErrorResponse(bre::HttpStatus::FORBIDDEN, "Access denied");

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("403 Forbidden") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_status_404) {
    auto response = bre::HttpResponse::MakeErrorResponse(bre::HttpStatus::NOT_FOUND);

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("404 Not Found") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_status_405) {
    auto response = bre::HttpResponse::MakeErrorResponse(bre::HttpStatus::METHOD_NOT_ALLOWED);

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("405 Method Not Allowed") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_status_409) {
    auto response = bre::HttpResponse::MakeErrorResponse(bre::HttpStatus::CONFLICT, "Conflict");

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("409 Conflict") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_status_500) {
    auto response = bre::HttpResponse::MakeErrorResponse(bre::HttpStatus::INTERNAL_SERVER_ERROR);

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("500 Internal Server Error") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_status_501) {
    auto response = bre::HttpResponse::MakeErrorResponse(bre::HttpStatus::NOT_IMPLEMENTED);

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("501 Not Implemented") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_status_503) {
    auto response = bre::HttpResponse::MakeErrorResponse(bre::HttpStatus::SERVICE_UNAVAILABLE,
                                                         "Try again later");

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("503 Service Unavailable") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_response_close_connection) {
    bre::HttpResponse response;
    response.SetStatus(bre::HttpStatus::OK);
    response.SetKeepAlive(false);
    response.SetBody("Close");

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("Connection: close") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_response_with_multiple_headers) {
    bre::HttpResponse response;
    response.SetStatus(bre::HttpStatus::OK);
    response.AddHeader("X-Custom-Header", "Value1");
    response.AddHeader("X-Another-Header", "Value2");
    response.AddHeader("X-Third-Header", "Value3");
    response.SetBody("Test");

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("X-Custom-Header: Value1") != std::string::npos);
    BOOST_CHECK(result.find("X-Another-Header: Value2") != std::string::npos);
    BOOST_CHECK(result.find("X-Third-Header: Value3") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_response_body_binary) {
    bre::HttpResponse response;
    response.SetStatus(bre::HttpStatus::OK);
    char data[] = {'\x00', '\x01', '\x02', '\x03', '\xFF', '\xFE', '\xFD'};
    std::string binaryData(data, 7);
    response.SetBody(binaryData);
    response.SetContentType("application/octet-stream");

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("Content-Length: 7") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_response_empty_body) {
    bre::HttpResponse response;
    response.SetStatus(bre::HttpStatus::NO_CONTENT);

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("Content-Length: 0") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_response_content_type_html) {
    bre::HttpResponse response;
    response.SetStatus(bre::HttpStatus::OK);
    response.SetBody("<html></html>");
    response.SetContentType("text/html");

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("Content-Type: text/html") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_response_content_type_json) {
    bre::HttpResponse response;
    response.SetStatus(bre::HttpStatus::OK);
    response.SetBody("{}");
    response.SetContentType("application/json");

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("Content-Type: application/json") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_response_content_type_css) {
    bre::HttpResponse response;
    response.SetStatus(bre::HttpStatus::OK);
    response.SetBody("body { color: red; }");
    response.SetContentType("text/css");

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("Content-Type: text/css") != std::string::npos);
}

BOOST_AUTO_TEST_CASE(test_response_content_type_js) {
    bre::HttpResponse response;
    response.SetStatus(bre::HttpStatus::OK);
    response.SetBody("console.log('test');");
    response.SetContentType("application/javascript");

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("Content-Type: application/javascript") != std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()
