#ifndef BRE_HTTP_REQUEST_HPP
#define BRE_HTTP_REQUEST_HPP

#include <string>
#include <unordered_map>
#include <string_view>
#include "../buffer/Buffer.hpp"

namespace bre {


/**
 * @brief HTTP请求方法
 */
enum class HttpMethod {
    INVALID,
    GET,
    POST,
    HEAD,
    PUT,
    DEL,    // DELETE is a macro on Windows
    OPTIONS,
    PATCH
};

/**
 * @brief HTTP解析状态
 */
enum class ParseState {
    REQUEST_LINE,
    HEADERS,
    BODY,
    FINISH
};

/**
 * @brief HTTP请求解析器
 */
class HttpRequest {
public:
    HttpRequest();
    ~HttpRequest() = default;

    /**
     * @brief 重置请求状态
     */
    void Reset();

    /**
     * @brief 解析HTTP请求
     * @param buffer 缓冲区
     * @return true表示解析完成，false表示需要更多数据
     */
    bool Parse(Buffer& buffer);

    /**
     * @brief 是否解析完成
     */
    bool IsFinished() const;

    // Getters
    HttpMethod GetMethod() const;
    std::string GetMethodString() const;
    const std::string& GetPath() const;
    const std::string& GetVersion() const;
    const std::string& GetBody() const;
    std::string GetHeader(const std::string& key) const;
    const std::unordered_map<std::string, std::string>& GetHeaders() const;
    bool HasHeader(const std::string& key) const;

private:
    bool _parseRequestLine(const char* begin, const char* end);
    bool _parseHeader(const char* begin, const char* end);
    HttpMethod _stringToMethod(const std::string& str);

    HttpMethod _method;
    std::string _path;
    std::string _version;
    std::unordered_map<std::string, std::string> _headers;
    std::string _body;
    ParseState _parseState;
    size_t _contentLength;
};

} // namespace bre

#endif // BRE_HTTP_REQUEST_HPP
