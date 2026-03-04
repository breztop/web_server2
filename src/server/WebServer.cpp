#include "WebServer.hpp"

#include <csignal>
#include <iostream>

#include "breutil/ini/ini.hpp"

namespace bre {

const std::string DEFAULT_CONFIG_FILE = "config.txt";

WebServer::WebServer()
    : _dbPool(PostgrePool::Instance())
    , _port(8080)
    , _ioPoolSize(4)
    , _threadPoolSize(4)
    , _running(false)
    , _totalConnections(0)
    , _activeConnections(0) {
    LoadConfig();
    InitDatabase();
    InitServer();
}

WebServer::~WebServer() { Stop(); }

void WebServer::LoadConfig() {
    auto& server_config = bre::Ini::Instance(DEFAULT_CONFIG_FILE)["server"];

    // 读取基本配置
    _port = static_cast<uint16_t>(server_config.GetInt("port", 8080));
    _resourceDir = server_config.GetStr("resource_dir", "../fronted");

    // IO Pool大小（默认为CPU核心数）
    int hardwareThreads = std::thread::hardware_concurrency();
    if (hardwareThreads <= 0) {
        hardwareThreads = 4;
    }
    _ioPoolSize = server_config.GetInt("io_pool_size", hardwareThreads);

    // 线程池大小
    _threadPoolSize = server_config.GetInt("thread_pool_size", 4);

    std::cout << "Configuration loaded:" << std::endl;
    std::cout << "127.0.0.1:" << _port << std::endl;
    std::cout << "  Resource Dir: " << _resourceDir << std::endl;
    std::cout << "  IO Pool Size: " << _ioPoolSize << std::endl;
    std::cout << "  Thread Pool Size: " << _threadPoolSize << std::endl;
}

void WebServer::InitDatabase() {
    auto& db_config = bre::Ini::Instance(DEFAULT_CONFIG_FILE)["database"];

    std::string dbHost = db_config.GetStr("DB_HOST", "localhost");
    std::string dbPort = db_config.GetStr("DB_PORT", "5432");
    std::string dbName = db_config.GetStr("DB_NAME", "");
    std::string dbUser = db_config.GetStr("DB_USER", "");
    std::string dbPassword = db_config.GetStr("DB_PASSWORD", "");

    if (!dbName.empty() && !dbUser.empty()) {
        // 构建连接字符串
        std::string connInfo =
            "host=" + dbHost + " port=" + dbPort + " dbname=" + dbName + " user=" + dbUser;

        if (!dbPassword.empty()) {
            connInfo += " password=" + dbPassword;
        }

        size_t dbPoolSize = static_cast<size_t>(db_config.GetInt("DB_POOL_SIZE", 8));

        try {
            _dbPool.Init(connInfo, dbPoolSize);
            std::cout << "Database pool initialized (size: " << dbPoolSize << ")" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Failed to initialize database pool: " << e.what() << std::endl;
        }
    } else {
        std::cout << "Database not configured, skipping initialization" << std::endl;
    }
}

void WebServer::InitServer() {
    // 创建线程池
    _threadPool = std::make_unique<ThreadPool>(_threadPoolSize);

    // 创建Acceptor
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), _port);

    _acceptor = std::make_unique<boost::asio::ip::tcp::acceptor>(_acceptorIoContext);
    _acceptor->open(endpoint.protocol());
    _acceptor->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
    _acceptor->bind(endpoint);
    _acceptor->listen();

    std::cout << "Server initialized on port " << _port << std::endl;
}

void WebServer::Start() {
    if (_running.exchange(true)) {
        std::cout << "Server is already running" << std::endl;
        return;
    }

    std::cout << "\n========================================" << std::endl;
    std::cout << "  Bre WebServer 2.0 Starting..." << std::endl;
    std::cout << "========================================" << std::endl;

    // 开始接受连接
    StartAccept();

    std::cout << "Server started successfully!" << std::endl;
    std::cout << "Listening 127.0.0.1:" << _port << std::endl;
    std::cout << "Press Ctrl+C to stop..." << std::endl;

    // 运行主Acceptor IO Context
    _acceptorIoContext.run();
}

void WebServer::Stop() {
    if (!_running.exchange(false)) {
        return;
    }

    std::cout << "\nShutting down server..." << std::endl;
    PrintStats();

    // 1. 停止接受新连接
    if (_acceptor) {
        boost::system::error_code ec;
        _acceptor->close(ec);
    }

    // 2. 停止主IO Context
    _acceptorIoContext.stop();

    // 停止线程池
    _threadPool.reset();

    // 关闭数据库连接池
    _dbPool.Close();

    std::cout << "Server stopped." << std::endl;
}

void WebServer::StartAccept() {
    _acceptor->async_accept(
        [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
            if (!ec) {
                ++_totalConnections;
                ++_activeConnections;

                std::cout << "New connection #" << _totalConnections.load() << " from "
                          << socket.remote_endpoint().address().to_string() << ":"
                          << socket.remote_endpoint().port() << std::endl;

                // 从Pool中获取io_context
                auto& ioContext = bre::AsioIOContextPool::Instance()->GetIOContext();

                // 将socket和session的创建post到目标io_context中执行
                // 这样session及其所有异步操作都会在对应的io_context线程中运行
                boost::asio::post(
                    ioContext, [socket = std::move(socket), resourceDir = _resourceDir]() mutable {
                        // 在目标io_context中创建并启动Session
                        auto session = std::make_shared<Session>(std::move(socket), resourceDir);
                        session->Start();
                    });
            } else {
                std::cerr << "Accept error: " << ec.message() << std::endl;
            }

            // 继续接受新连接
            if (_running) {
                StartAccept();
            }
        });
}

void WebServer::HandleAccept(boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
    if (!ec) {
        ++_totalConnections;
        ++_activeConnections;

        // 从Pool中获取io_context
        auto& ioContext = bre::AsioIOContextPool::Instance()->GetIOContext();

        // 将socket和session的创建post到目标io_context中执行
        // 这样session就会在对应的io_context线程中运行
        boost::asio::post(
            ioContext, [socket = std::move(socket), resourceDir = _resourceDir, this]() mutable {
                // 在目标io_context中创建并启动Session
                auto session = std::make_shared<Session>(std::move(socket), resourceDir);
                session->Start();
            });
    }

    // 继续接受连接
    if (_running) {
        StartAccept();
    }
}

void WebServer::PrintStats() const {
    std::cout << "\n========== Server Statistics ==========" << std::endl;
    std::cout << "Total Connections: " << _totalConnections.load() << std::endl;
    std::cout << "Active Connections: " << _activeConnections.load() << std::endl;
    std::cout << "Thread Pool Size: " << _threadPool->GetThreadCount() << std::endl;
    std::cout << "=======================================" << std::endl;
}

}  // namespace bre
