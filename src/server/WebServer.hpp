#pragma once

#include <atomic>
#include <boost/asio.hpp>
#include <memory>
#include <string>

#include "../database/PostgrePool.hpp"
#include "Session.hpp"
#include "breutil/net/asio_io_context_pool.hpp"
#include "breutil/thread_pool.hpp"

namespace bre {

/**
 * @brief WebServer 主服务器类
 */
class WebServer {
public:
    /**
     * @brief 构造函数
     */
    WebServer();

    /**
     * @brief 析构函数
     */
    ~WebServer();

    // 禁止拷贝和赋值
    WebServer(const WebServer&) = delete;
    WebServer& operator=(const WebServer&) = delete;

    /**
     * @brief 启动服务器
     */
    void Start();

    /**
     * @brief 停止服务器
     */
    void Stop();

    /**
     * @brief 获取服务器统计信息
     */
    void PrintStats() const;

private:
    /**
     * @brief 加载配置
     */
    void LoadConfig();

    /**
     * @brief 初始化数据库
     */
    void InitDatabase();

    /**
     * @brief 初始化服务器
     */
    void InitServer();

    /**
     * @brief 开始接受连接
     */
    void StartAccept();

    /**
     * @brief 处理新连接
     */
    void HandleAccept(boost::system::error_code ec, boost::asio::ip::tcp::socket socket);

    // 主IO Context（只用于Accept）
    boost::asio::io_context _acceptorIoContext;

    // Acceptor
    std::unique_ptr<boost::asio::ip::tcp::acceptor> _acceptor;


    // 线程池（用于CPU密集型任务）
    std::unique_ptr<ThreadPool> _threadPool;

    // 数据库连接池引用
    PostgrePool& _dbPool;

    // 配置
    uint16_t _port;
    std::string _resourceDir;
    size_t _ioPoolSize;
    size_t _threadPoolSize;

    // 运行状态
    std::atomic<bool> _running;

    // 统计信息
    std::atomic<uint64_t> _totalConnections;
    std::atomic<uint64_t> _activeConnections;
};

}  // namespace bre
