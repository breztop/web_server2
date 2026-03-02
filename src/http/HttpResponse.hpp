#pragma once

#include <string>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <sstream>
#include "../buffer/Buffer.hpp"

namespace bre {

/**
 * @brief HTTP状态码
 */
enum class HttpStatus {
    OK = 200,
    CREATED = 201,
    NO_CONTENT = 204,
    MOVED_PERMANENTLY = 301,
    FOUND = 302,
    NOT_MODIFIED = 304,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    SERVICE_UNAVAILABLE = 503
};

/**
 * @brief HTTP响应构建器
 */
class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse() = default;

    /**
     * @brief 设置状态码
     */
    void SetStatus(HttpStatus status);

    /**
     * @brief 设置响应体
     */
    void SetBody(const std::string& body);

    void SetBody(std::string&& body);

    /**
     * @brief 添加响应头
     */
    void AddHeader(const std::string& key, const std::string& value);

    /**
     * @brief 设置Keep-Alive
     */
    void SetKeepAlive(bool keepAlive);

    /**
     * @brief 设置内容类型
     */
    void SetContentType(const std::string& contentType);

    /**
     * @brief 从文件加载响应体
     * @param filePath 文件路径
     * @return 成功返回true
     */
    bool LoadFile(const std::filesystem::path& filePath);

    /**
     * @brief 构建完整的HTTP响应
     * @param buffer 输出缓冲区
     */
    void Build(Buffer& buffer);

    /**
     * @brief 构建错误响应
     */
    static HttpResponse MakeErrorResponse(HttpStatus status, const std::string& message = "");

    /**
     * @brief 构建简单文本响应
     */
    static HttpResponse MakeTextResponse(const std::string& text);

    /**
     * @brief 构建JSON响应
     */
    static HttpResponse MakeJsonResponse(const std::string& json);

private:
    std::string _getStatusMessage(HttpStatus status);
    std::string _getContentType(const std::string& ext);

private:
    HttpStatus _status;
    std::unordered_map<std::string, std::string> _headers;
    std::string _body;
    bool _keepAlive;
};

} // namespace bre

