#include "HttpRequest.hpp"
#include <algorithm>

namespace bre {

HttpRequest::HttpRequest() {
    Reset();
}

void HttpRequest::Reset() {
    _method = HttpMethod::INVALID;
    _path.clear();
    _version.clear();
    _headers.clear();
    _body.clear();
    _parseState = ParseState::REQUEST_LINE;
    _contentLength = 0;
}

bool HttpRequest::Parse(Buffer& buffer) {
    const char* crlf = nullptr;
    
    while (true) {
        if (_parseState == ParseState::REQUEST_LINE) {
            crlf = buffer.FindCRLF();
            if (!crlf) {
                return false; // 需要更多数据
            }
            
            if (!_parseRequestLine(buffer.Peek(), crlf)) {
                return false;
            }
            
            buffer.RetrieveUntil(crlf + 2);
            _parseState = ParseState::HEADERS;
        }
        else if (_parseState == ParseState::HEADERS) {
            crlf = buffer.FindCRLF();
            if (!crlf) {
                return false;
            }
            
            // 空行表示头部结束
            if (crlf == buffer.Peek()) {
                buffer.RetrieveUntil(crlf + 2);
                
                // 检查是否有body
                auto it = _headers.find("Content-Length");
                if (it != _headers.end()) {
                    _contentLength = std::stoull(it->second);
                    _parseState = ParseState::BODY;
                } else {
                    _parseState = ParseState::FINISH;
                    return true;
                }
            }
            else {
                if (!_parseHeader(buffer.Peek(), crlf)) {
                    return false;
                }
                buffer.RetrieveUntil(crlf + 2);
            }
        }
        else if (_parseState == ParseState::BODY) {
            if (buffer.ReadableBytes() >= _contentLength) {
                _body = buffer.RetrieveAsString(_contentLength);
                _parseState = ParseState::FINISH;
                return true;
            }
            return false;
        }
        else if (_parseState == ParseState::FINISH) {
            return true;
        }
    }
    
    return false;
}

bool HttpRequest::IsFinished() const {
    return _parseState == ParseState::FINISH;
}

HttpMethod HttpRequest::GetMethod() const {
    return _method;
}

std::string HttpRequest::GetMethodString() const {
    switch (_method) {
        case HttpMethod::GET: return "GET";
        case HttpMethod::POST: return "POST";
        case HttpMethod::HEAD: return "HEAD";
        case HttpMethod::PUT: return "PUT";
        case HttpMethod::DEL: return "delete";
        case HttpMethod::OPTIONS: return "OPTIONS";
        case HttpMethod::PATCH: return "PATCH";
        default: return "INVALID";
    }
}

const std::string& HttpRequest::GetPath() const {
    return _path;
}

const std::string& HttpRequest::GetVersion() const {
    return _version;
}

const std::string& HttpRequest::GetBody() const {
    return _body;
}

std::string HttpRequest::GetHeader(const std::string& key) const {
    auto it = _headers.find(key);
    return it != _headers.end() ? it->second : "";
}

const std::unordered_map<std::string, std::string>& HttpRequest::GetHeaders() const {
    return _headers;
}

bool HttpRequest::HasHeader(const std::string& key) const {
    return _headers.find(key) != _headers.end();
}

bool HttpRequest::_parseRequestLine(const char* begin, const char* end) {
    // 解析: METHOD PATH VERSION\r\n
    const char* space1 = std::find(begin, end, ' ');
    if (space1 == end) return false;

    // 解析方法
    std::string methodStr(begin, space1);
    _method = _stringToMethod(methodStr);
    if (_method == HttpMethod::INVALID) {
        return false;
    }

    // 解析路径
    const char* space2 = std::find(space1 + 1, end, ' ');
    if (space2 == end) return false;

    _path = std::string(space1 + 1, space2);
    
    // 解析版本
    _version = std::string(space2 + 1, end);
    
    return true;
}

bool HttpRequest::_parseHeader(const char* begin, const char* end) {
    // 解析: Key: Value\r\n
    const char* colon = std::find(begin, end, ':');
    if (colon == end) return false;

    std::string key(begin, colon);
    
    // 跳过冒号后的空格
    const char* valueStart = colon + 1;
    while (valueStart < end && *valueStart == ' ') {
        ++valueStart;
    }

    std::string value(valueStart, end);
    _headers[key] = value;
    
    return true;
}

HttpMethod HttpRequest::_stringToMethod(const std::string& str) {
    if (str == "GET") return HttpMethod::GET;
    if (str == "POST") return HttpMethod::POST;
    if (str == "HEAD") return HttpMethod::HEAD;
    if (str == "PUT") return HttpMethod::PUT;
    if (str == "DELETE") return HttpMethod::DEL;
    if (str == "OPTIONS") return HttpMethod::OPTIONS;
    if (str == "PATCH") return HttpMethod::PATCH;
    return HttpMethod::INVALID;
}

} // namespace bre
