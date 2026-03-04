#include "HttpConnection.hpp"
#include <filesystem>
#include <iostream>

namespace bre {

HttpConnection::HttpConnection(boost::asio::ip::tcp::socket socket, 
                               const std::string& resourceDir)
    : _socket(std::move(socket)),
      _resourceDir(resourceDir)
    {}

void HttpConnection::Start() {
    _doRead();
}

std::string HttpConnection::GetRemoteAddress() const {
    try {
        return _socket.remote_endpoint().address().to_string();
    } catch (...) {
        return "unknown";
    }
}

void HttpConnection::_doRead() {
    auto self = shared_from_this();
    
    _socket.async_read_some(
        boost::asio::buffer(_readBuffer.BeginWrite(), _readBuffer.WritableBytes()),
        [this, self](boost::system::error_code ec, std::size_t bytesTransferred) {
            if (!ec) {
                _readBuffer.HasWritten(bytesTransferred);
                
                // 解析HTTP请求
                if (_request.Parse(_readBuffer)) {
                    _handleRequest();
                } else {
                    // 需要更多数据
                    _readBuffer.EnsureWritableBytes(1024);
                    _doRead();
                }
            } else {
                std::cerr << "Read error: " << ec.message() << std::endl;
            }
        });
}

void HttpConnection::_handleRequest() {
    HttpResponse response;
    response.SetKeepAlive(_request.GetHeader("Connection") == "keep-alive");

    // 获取请求路径
    std::string path = _request.GetPath();
    if (path == "/") {
        path = "/index.html";
    }

    // 构建完整文件路径
    std::filesystem::path filePath = std::filesystem::path(_resourceDir) / path.substr(1);

    // 检查文件是否存在
    if (std::filesystem::exists(filePath) && std::filesystem::is_regular_file(filePath)) {
        if (response.LoadFile(filePath)) {
            response.SetStatus(HttpStatus::OK);
            std::cout << "Serving: " << filePath << std::endl;
        } else {
            response = HttpResponse::MakeErrorResponse(HttpStatus::FORBIDDEN);
        }
    } else {
        response = HttpResponse::MakeErrorResponse(HttpStatus::NOT_FOUND);
    }

    // 构建响应并发送
    Buffer writeBuffer;
    response.Build(writeBuffer);
    _doWrite(writeBuffer);
}

void HttpConnection::_doWrite(Buffer& buffer) {
    auto self = shared_from_this();
    
    boost::asio::async_write(
        _socket,
        boost::asio::buffer(buffer.Peek(), buffer.ReadableBytes()),
        [this, self](boost::system::error_code ec, std::size_t) {
            if (!ec) {
                // 写入成功，关闭连接（简化版本，不支持keep-alive）
                boost::system::error_code ignored_ec;
                _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
            } else {
                std::cerr << "Write error: " << ec.message() << std::endl;
            }
        });
}

} // namespace bre
