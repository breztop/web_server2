#define BOOST_TEST_MODULE ContentTypeTest
#include <boost/test/included/unit_test.hpp>
#include <filesystem>
#include <fstream>

#include "breutil/buffer.hpp"
#include "http/HttpResponse.hpp"

BOOST_AUTO_TEST_SUITE(ContentTypeTestSuite)

struct TempFileFixture {
    std::string tempDir = "test_temp_files";

    TempFileFixture() { std::filesystem::create_directory(tempDir); }

    ~TempFileFixture() { std::filesystem::remove_all(tempDir); }

    std::string createFile(const std::string& filename, const std::string& content) {
        std::string path = tempDir + "/" + filename;
        std::ofstream file(path);
        file << content;
        file.close();
        return path;
    }
};

BOOST_FIXTURE_TEST_CASE(test_content_type_html, TempFileFixture) {
    bre::HttpResponse response;
    std::string path = createFile("test.html", "<html></html>");
    BOOST_CHECK(response.LoadFile(path));

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("text/html") != std::string::npos);
}

BOOST_FIXTURE_TEST_CASE(test_content_type_htm, TempFileFixture) {
    bre::HttpResponse response;
    std::string path = createFile("test.htm", "<html></html>");
    BOOST_CHECK(response.LoadFile(path));

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("text/html") != std::string::npos);
}

BOOST_FIXTURE_TEST_CASE(test_content_type_css, TempFileFixture) {
    bre::HttpResponse response;
    std::string path = createFile("test.css", "body { }");
    BOOST_CHECK(response.LoadFile(path));

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("text/css") != std::string::npos);
}

BOOST_FIXTURE_TEST_CASE(test_content_type_js, TempFileFixture) {
    bre::HttpResponse response;
    std::string path = createFile("test.js", "console.log('test');");
    BOOST_CHECK(response.LoadFile(path));

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("application/javascript") != std::string::npos);
}

BOOST_FIXTURE_TEST_CASE(test_content_type_json, TempFileFixture) {
    bre::HttpResponse response;
    std::string path = createFile("test.json", "{}");
    BOOST_CHECK(response.LoadFile(path));

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("application/json") != std::string::npos);
}

BOOST_FIXTURE_TEST_CASE(test_content_type_xml, TempFileFixture) {
    bre::HttpResponse response;
    std::string path = createFile("test.xml", "<?xml version='1.0'?><root/>");
    BOOST_CHECK(response.LoadFile(path));

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("application/xml") != std::string::npos);
}

BOOST_FIXTURE_TEST_CASE(test_content_type_jpg, TempFileFixture) {
    bre::HttpResponse response;
    std::string path = createFile("test.jpg", "fake jpg data");
    BOOST_CHECK(response.LoadFile(path));

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("image/jpeg") != std::string::npos);
}

BOOST_FIXTURE_TEST_CASE(test_content_type_jpeg, TempFileFixture) {
    bre::HttpResponse response;
    std::string path = createFile("test.jpeg", "fake jpeg data");
    BOOST_CHECK(response.LoadFile(path));

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("image/jpeg") != std::string::npos);
}

BOOST_FIXTURE_TEST_CASE(test_content_type_png, TempFileFixture) {
    bre::HttpResponse response;
    std::string path = createFile("test.png", "fake png data");
    BOOST_CHECK(response.LoadFile(path));

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("image/png") != std::string::npos);
}

BOOST_FIXTURE_TEST_CASE(test_content_type_gif, TempFileFixture) {
    bre::HttpResponse response;
    std::string path = createFile("test.gif", "fake gif data");
    BOOST_CHECK(response.LoadFile(path));

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("image/gif") != std::string::npos);
}

BOOST_FIXTURE_TEST_CASE(test_content_type_svg, TempFileFixture) {
    bre::HttpResponse response;
    std::string path = createFile("test.svg", "<svg></svg>");
    BOOST_CHECK(response.LoadFile(path));

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("image/svg+xml") != std::string::npos);
}

BOOST_FIXTURE_TEST_CASE(test_content_type_ico, TempFileFixture) {
    bre::HttpResponse response;
    std::string path = createFile("test.ico", "fake ico data");
    BOOST_CHECK(response.LoadFile(path));

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("image/x-icon") != std::string::npos);
}

BOOST_FIXTURE_TEST_CASE(test_content_type_txt, TempFileFixture) {
    bre::HttpResponse response;
    std::string path = createFile("test.txt", "plain text");
    BOOST_CHECK(response.LoadFile(path));

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("text/plain") != std::string::npos);
}

BOOST_FIXTURE_TEST_CASE(test_content_type_pdf, TempFileFixture) {
    bre::HttpResponse response;
    std::string path = createFile("test.pdf", "fake pdf data");
    BOOST_CHECK(response.LoadFile(path));

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("application/pdf") != std::string::npos);
}

BOOST_FIXTURE_TEST_CASE(test_content_type_zip, TempFileFixture) {
    bre::HttpResponse response;
    std::string path = createFile("test.zip", "fake zip data");
    BOOST_CHECK(response.LoadFile(path));

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("application/zip") != std::string::npos);
}

BOOST_FIXTURE_TEST_CASE(test_content_type_mp4, TempFileFixture) {
    bre::HttpResponse response;
    std::string path = createFile("test.mp4", "fake mp4 data");
    BOOST_CHECK(response.LoadFile(path));

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("video/mp4") != std::string::npos);
}

BOOST_FIXTURE_TEST_CASE(test_content_type_mp3, TempFileFixture) {
    bre::HttpResponse response;
    std::string path = createFile("test.mp3", "fake mp3 data");
    BOOST_CHECK(response.LoadFile(path));

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("audio/mpeg") != std::string::npos);
}

BOOST_FIXTURE_TEST_CASE(test_content_type_unknown, TempFileFixture) {
    bre::HttpResponse response;
    std::string path = createFile("test.xyz", "unknown type");
    BOOST_CHECK(response.LoadFile(path));

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("application/octet-stream") != std::string::npos);
}

BOOST_FIXTURE_TEST_CASE(test_load_file_content_length, TempFileFixture) {
    bre::HttpResponse response;
    std::string content = "Test content here";
    std::string path = createFile("test.txt", content);
    BOOST_CHECK(response.LoadFile(path));

    bre::Buffer buffer;
    response.Build(buffer);

    std::string result = buffer.ToString();
    BOOST_CHECK(result.find("Content-Length: " + std::to_string(content.size())) !=
                std::string::npos);
}

BOOST_AUTO_TEST_SUITE_END()
