#ifndef BRE_SESSION_HPP
#define BRE_SESSION_HPP

#include <boost/asio.hpp>
#include <memory>
#include <string>
#include <chrono>
#include "../buffer/Buffer.hpp"
#include "../http/HttpRequest.hpp"
#include "../http/HttpResponse.hpp"

namespace bre {

/**
 * @brief Session类 - 管理单个TCP连接
 * 负责HTTP请求的接收、解析、处理和响应
 */
class Session : public std::enable_shared_from_this<Session> {
public:
    /**
     * @brief 构造函数
     * @param socket TCP socket
     * @param resourceDir 静态资源目录
     */
    Session(boost::asio::ip::tcp::socket socket, 
            const std::string& resourceDir);

    /**
     * @brief 析构函数
     */
    ~Session();

    /**
     * @brief 启动Session（开始处理请求）
     */
    void Start();

    /**
     * @brief 停止Session
     */
    void Stop();

    /**
     * @brief 获取客户端地址
     */
    std::string GetRemoteAddress() const;

    /**
     * @brief 获取Session ID
     */
    uint64_t GetSessionId() const { return _sessionId; }

private:
    /**
     * @brief 异步读取数据
     */
    void DoRead();

    /**
     * @brief 异步写入响应
     */
    void DoWrite();

    /**
     * @brief 处理HTTP请求
     */
    void HandleRequest();

    /**
     * @brief 处理超时
     */
    void HandleTimeout();

    /**
     * @brief 启动超时定时器
     */
    void StartTimer();

    /**
     * @brief 重置超时定时器
     */
    void ResetTimer();

    /**
     * @brief 取消定时器
     */
    void CancelTimer();

    // TCP socket
    boost::asio::ip::tcp::socket _socket;
    
    // 超时定时器
    boost::asio::steady_timer _timer;
    
    // 读缓冲区
    Buffer _readBuffer;
    
    // 写缓冲区
    Buffer _writeBuffer;
    
    // HTTP请求
    HttpRequest _request;
    
    // 静态资源目录
    std::string _resourceDir;
    
    // Session ID
    uint64_t _sessionId;
    
    // 超时时间
    std::chrono::seconds _timeoutDuration;
    
    // Keep-Alive标志
    bool _keepAlive;
    
    // 是否已关闭
    bool _closed;
    
    // 全局Session计数器
    static std::atomic<uint64_t> _sessionCounter;
};

} // namespace bre

#endif // BRE_SESSION_HPP
