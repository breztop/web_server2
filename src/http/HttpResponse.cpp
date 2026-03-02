#include "HttpResponse.hpp"
#include <fstream>
#include <sstream>

namespace bre {

HttpResponse::HttpResponse() 
    : _status(HttpStatus::OK), _keepAlive(false) {}

void HttpResponse::SetStatus(HttpStatus status) {
    _status = status;
}

void HttpResponse::SetBody(const std::string& body) {
    _body = body;
}

void HttpResponse::SetBody(std::string&& body) {
    _body = std::move(body);
}

void HttpResponse::AddHeader(const std::string& key, const std::string& value) {
    _headers[key] = value;
}

void HttpResponse::SetKeepAlive(bool keepAlive) {
    _keepAlive = keepAlive;
}

void HttpResponse::SetContentType(const std::string& contentType) {
    AddHeader("Content-Type", contentType);
}

bool HttpResponse::LoadFile(const std::filesystem::path& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    _body = buffer.str();

    // 自动设置Content-Type
    std::string ext = filePath.extension().string();
    SetContentType(_getContentType(ext));

    return true;
}

void HttpResponse::Build(Buffer& buffer) {
    // 状态行
    std::string statusLine = "HTTP/1.1 " + std::to_string(static_cast<int>(_status)) 
                           + " " + _getStatusMessage(_status) + "\r\n";
    buffer.Append(statusLine);

    // 响应头
    if (_keepAlive) {
        AddHeader("Connection", "keep-alive");
        AddHeader("Keep-Alive", "timeout=5, max=100");
    } else {
        AddHeader("Connection", "close");
    }

    AddHeader("Content-Length", std::to_string(_body.size()));

    for (const auto& [key, value] : _headers) {
        buffer.Append(key + ": " + value + "\r\n");
    }

    buffer.Append("\r\n");

    // 响应体
    if (!_body.empty()) {
        buffer.Append(_body);
    }
}

HttpResponse HttpResponse::MakeErrorResponse(HttpStatus status, const std::string& message) {
    HttpResponse response;
    response.SetStatus(status);
    response.SetContentType("text/html; charset=utf-8");

    std::string body = "<html><head><title>Error</title></head><body>";
    body += "<h1>" + std::to_string(static_cast<int>(status)) + " " 
          + response._getStatusMessage(status) + "</h1>";
    if (!message.empty()) {
        body += "<p>" + message + "</p>";
    }
    body += "</body></html>";

    response.SetBody(body);
    return response;
}

HttpResponse HttpResponse::MakeTextResponse(const std::string& text) {
    HttpResponse response;
    response.SetStatus(HttpStatus::OK);
    response.SetContentType("text/plain; charset=utf-8");
    response.SetBody(text);
    return response;
}

HttpResponse HttpResponse::MakeJsonResponse(const std::string& json) {
    HttpResponse response;
    response.SetStatus(HttpStatus::OK);
    response.SetContentType("application/json; charset=utf-8");
    response.SetBody(json);
    return response;
}

std::string HttpResponse::_getStatusMessage(HttpStatus status) {
    switch (status) {
        case HttpStatus::OK: return "OK";
        case HttpStatus::CREATED: return "Created";
        case HttpStatus::NO_CONTENT: return "No Content";
        case HttpStatus::MOVED_PERMANENTLY: return "Moved Permanently";
        case HttpStatus::FOUND: return "Found";
        case HttpStatus::NOT_MODIFIED: return "Not Modified";
        case HttpStatus::BAD_REQUEST: return "Bad Request";
        case HttpStatus::UNAUTHORIZED: return "Unauthorized";
        case HttpStatus::FORBIDDEN: return "Forbidden";
        case HttpStatus::NOT_FOUND: return "Not Found";
        case HttpStatus::METHOD_NOT_ALLOWED: return "Method Not Allowed";
        case HttpStatus::INTERNAL_SERVER_ERROR: return "Internal Server Error";
        case HttpStatus::NOT_IMPLEMENTED: return "Not Implemented";
        case HttpStatus::SERVICE_UNAVAILABLE: return "Service Unavailable";
        default: return "Unknown";
    }
}

std::string HttpResponse::_getContentType(const std::string& ext) {
    static const std::unordered_map<std::string, std::string> mimeTypes = {
        {".html", "text/html; charset=utf-8"},
        {".htm", "text/html; charset=utf-8"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".json", "application/json"},
        {".xml", "application/xml"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".png", "image/png"},
        {".gif", "image/gif"},
        {".svg", "image/svg+xml"},
        {".ico", "image/x-icon"},
        {".txt", "text/plain"},
        {".pdf", "application/pdf"},
        {".zip", "application/zip"},
        {".mp4", "video/mp4"},
        {".mp3", "audio/mpeg"}
    };

    auto it = mimeTypes.find(ext);
    return it != mimeTypes.end() ? it->second : "application/octet-stream";
}

} // namespace bre
