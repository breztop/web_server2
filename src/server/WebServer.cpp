#include "WebServer.hpp"
#include <iostream>
#include <csignal>

namespace bre {

WebServer::WebServer()
    : _dbPool(PostgreSQLPool::GetInstance()),
      _port(8080),
      _ioPoolSize(4),
      _threadPoolSize(4),
      _running(false),
      _totalConnections(0),
      _activeConnections(0) {
    
    LoadConfig();
    InitDatabase();
    InitServer();
}

WebServer::~WebServer() {
    Stop();
}

void WebServer::LoadConfig() {
    auto& config = Config::GetInstance();
    
    // 加载配置文件
    if (!config.LoadFromFile("config.txt")) {
        std::cout << "Config file not found, using default values" << std::endl;
    }

    config.LoadFromEnv();  // 从环境变量覆盖配置

    // 读取基本配置
    _port = static_cast<uint16_t>(std::stoi(config.GetOrDefault("PORT", "8080")));
    _resourceDir = config.GetOrDefault("RESOURCE_DIR", "../fronted");
    
    // IO Pool大小（默认为CPU核心数）
    _ioPoolSize = std::stoul(config.GetOrDefault("IO_POOL_SIZE", "0"));
    if (_ioPoolSize == 0) {
        _ioPoolSize = std::thread::hardware_concurrency();
        if (_ioPoolSize == 0) _ioPoolSize = 4;
    }
    
    // 线程池大小
    _threadPoolSize = std::stoul(config.GetOrDefault("THREAD_POOL_SIZE", "4"));
    
    std::cout << "Configuration loaded:" << std::endl;
    std::cout << "127.0.0.1:" << _port << std::endl;
    std::cout << "  Resource Dir: " << _resourceDir << std::endl;
    std::cout << "  IO Pool Size: " << _ioPoolSize << std::endl;
    std::cout << "  Thread Pool Size: " << _threadPoolSize << std::endl;
}

void WebServer::InitDatabase() {
    auto& config = Config::GetInstance();
    
    // 从环境变量读取数据库密码（安全实践）
    std::string dbHost = config.GetOrDefault("DB_HOST", "localhost");
    std::string dbPort = config.GetOrDefault("DB_PORT", "5432");
    std::string dbName = config.GetOrDefault("DB_NAME", "");
    std::string dbUser = config.GetOrDefault("DB_USER", "");
    std::string dbPassword = config.GetOrDefault("DB_PASSWORD", "");  // 从环境变量读取
    
    if (!dbName.empty() && !dbUser.empty()) {
        // 构建连接字符串
        std::string connInfo = "host=" + dbHost + 
                              " port=" + dbPort +
                              " dbname=" + dbName +
                              " user=" + dbUser;
        
        if (!dbPassword.empty()) {
            connInfo += " password=" + dbPassword;
        }
        
        size_t dbPoolSize = std::stoul(config.GetOrDefault("DB_POOL_SIZE", "8"));
        
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
    // 创建IO Context Pool
    _ioPool = std::make_unique<IoContextPool>(_ioPoolSize);
    
    // 创建线程池
    _threadPool = std::make_unique<ThreadPool>(_threadPoolSize);
    
    // 创建Acceptor
    boost::asio::ip::tcp::endpoint endpoint(
        boost::asio::ip::tcp::v4(), _port);
    
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
    
    // 启动IO Context Pool
    _ioPool->Start();
    
    // 开始接受连接
    StartAccept();
    
    std::cout << "Server started successfully!" << std::endl;
    std::cout << "Listening on port " << _port << std::endl;
    std::cout << "Press Ctrl+C to stop..." << std::endl;
    
    // 运行主Acceptor IO Context
    _acceptorIoContext.run();
}

void WebServer::Stop() {
    if (!_running.exchange(false)) {
        return;
    }
    
    std::cout << "\nShutting down server..." << std::endl;
    
    // 1. 停止接受新连接
    if (_acceptor) {
        boost::system::error_code ec;
        _acceptor->close(ec);
    }
    
    // 2. 停止主IO Context
    _acceptorIoContext.stop();
    
    // 3. 停止IO Pool（会等待所有Session完成）
    if (_ioPool) {
        _ioPool->Stop();
    }
    
    // 4. 停止线程池
    _threadPool.reset();
    
    // 5. 关闭数据库连接池
    _dbPool.Close();
    
    PrintStats();
    std::cout << "Server stopped." << std::endl;
}

void WebServer::StartAccept() {
    _acceptor->async_accept(
        [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
            if (!ec) {
                ++_totalConnections;
                ++_activeConnections;
                
                std::cout << "New connection #" << _totalConnections.load() 
                         << " from " << socket.remote_endpoint().address().to_string() 
                         << ":" << socket.remote_endpoint().port() << std::endl;
                
                // 从Pool中获取io_context
                auto& ioContext = _ioPool->GetIoContext();
                
                // 将socket和session的创建post到目标io_context中执行
                // 这样session及其所有异步操作都会在对应的io_context线程中运行
                boost::asio::post(ioContext, [socket = std::move(socket), 
                                               resourceDir = _resourceDir]() mutable {
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

void WebServer::HandleAccept(boost::system::error_code ec, 
                             boost::asio::ip::tcp::socket socket) {
    if (!ec) {
        ++_totalConnections;
        ++_activeConnections;
        
        // 从Pool中获取io_context
        auto& ioContext = _ioPool->GetIoContext();
        
        // 将socket和session的创建post到目标io_context中执行
        // 这样session就会在对应的io_context线程中运行
        boost::asio::post(ioContext, [socket = std::move(socket), 
                                       resourceDir = _resourceDir, 
                                       this]() mutable {
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
    std::cout << "IO Pool Size: " << _ioPool->GetPoolSize() << std::endl;
    std::cout << "Thread Pool Size: " << _threadPool->GetThreadCount() << std::endl;
    std::cout << "=======================================" << std::endl;
}

} // namespace bre
