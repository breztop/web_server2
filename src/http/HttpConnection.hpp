#ifndef BRE_HTTP_CONNECTION_HPP
#define BRE_HTTP_CONNECTION_HPP

#include <boost/asio.hpp>
#include <memory>
#include <filesystem>
#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "../buffer/Buffer.hpp"

namespace bre {

/**
 * @brief HTTP连接处理类
 * 管理单个HTTP连接的生命周期
 */
class HttpConnection : public std::enable_shared_from_this<HttpConnection> {
public:
    HttpConnection(boost::asio::ip::tcp::socket socket, 
                   const std::string& resourceDir);

    /**
     * @brief 开始处理连接
     */
    void Start();

    /**
     * @brief 获取远程地址
     */
    std::string GetRemoteAddress() const;

private:
    void _doRead();

    void _handleRequest();

    void _doWrite(Buffer& buffer);

private:
    boost::asio::ip::tcp::socket _socket;
    Buffer _readBuffer{4096};
    HttpRequest _request;
    std::string _resourceDir;
};

} // namespace bre

#endif // BRE_HTTP_CONNECTION_HPP
