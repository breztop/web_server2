#ifndef BRE_POSTGRESQL_POOL_HPP
#define BRE_POSTGRESQL_POOL_HPP

#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <stdexcept>
#include <iostream>

// 跨平台PostgreSQL支持
#ifdef _WIN32
    // Windows下暂时不链接PostgreSQL，仅提供接口
    #define PG_DISABLED
#else
    // Linux下使用libpq
    #include <postgresql/libpq-fe.h>
#endif

namespace bre {

#ifdef PG_DISABLED
// Windows下的占位类型
struct PGconn {};
struct PGresult {};

// 占位函数
inline PGconn* PQconnectdb(const char*) { return nullptr; }
inline void PQfinish(PGconn*) {}
inline PGresult* PQexec(PGconn*, const char*) { return nullptr; }
inline void PQclear(PGresult*) {}
inline int PQstatus(PGconn*) { return 0; }
inline char* PQerrorMessage(PGconn*) { return const_cast<char*>("PG disabled on Windows"); }
#define CONNECTION_OK 0
#endif

/**
 * @brief PostgreSQL连接RAII封装
 */
class PGConnection {
public:
    explicit PGConnection(PGconn* conn) : _conn(conn) {}
    
    ~PGConnection() {
        if (_conn) {
            PQfinish(_conn);
            _conn = nullptr;
        }
    }

    // 禁止拷贝
    PGConnection(const PGConnection&) = delete;
    PGConnection& operator=(const PGConnection&) = delete;

    // 支持移动
    PGConnection(PGConnection&& other) noexcept : _conn(other._conn) {
        other._conn = nullptr;
    }

    PGConnection& operator=(PGConnection&& other) noexcept {
        if (this != &other) {
            if (_conn) {
                PQfinish(_conn);
            }
            _conn = other._conn;
            other._conn = nullptr;
        }
        return *this;
    }

    PGconn* Get() { return _conn; }
    const PGconn* Get() const { return _conn; }

    bool IsValid() const { 
#ifdef PG_DISABLED
        return false;
#else
        return _conn && PQstatus(_conn) == CONNECTION_OK; 
#endif
    }

private:
    PGconn* _conn;
};

/**
 * @brief PostgreSQL连接池
 * 管理数据库连接的创建、分配和回收
 */
class PostgreSQLPool {
public:
    /**
     * @brief 获取单例实例
     */
    static PostgreSQLPool& GetInstance() {
        static PostgreSQLPool instance;
        return instance;
    }

    // 禁止拷贝和赋值
    PostgreSQLPool(const PostgreSQLPool&) = delete;
    PostgreSQLPool& operator=(const PostgreSQLPool&) = delete;

    /**
     * @brief 初始化连接池
     * @param connInfo 连接字符串 (e.g., "host=localhost port=5432 dbname=mydb user=user password=pass")
     * @param poolSize 连接池大小
     */
    void Init(const std::string& connInfo, size_t poolSize = 8) {
        std::lock_guard<std::mutex> lock(_mutex);
        
        if (_initialized) {
            std::cerr << "PostgreSQLPool already initialized" << std::endl;
            return;
        }

        _connInfo = connInfo;
        _poolSize = poolSize;

#ifdef PG_DISABLED
        std::cout << "PostgreSQL is disabled on Windows (for development)" << std::endl;
        std::cout << "Connection info would be: " << connInfo << std::endl;
        _initialized = true;
        return;
#else
        // 创建连接
        for (size_t i = 0; i < poolSize; ++i) {
            PGconn* conn = PQconnectdb(connInfo.c_str());
            if (PQstatus(conn) != CONNECTION_OK) {
                std::cerr << "Failed to connect to database: " 
                         << PQerrorMessage(conn) << std::endl;
                PQfinish(conn);
                // 清理已创建的连接
                while (!_connections.empty()) {
                    _connections.pop();
                }
                throw std::runtime_error("Failed to initialize PostgreSQL connection pool");
            }
            _connections.push(std::make_unique<PGConnection>(conn));
        }
        _initialized = true;
        std::cout << "PostgreSQLPool initialized with " << poolSize << " connections" << std::endl;
#endif
    }

    /**
     * @brief 从连接池获取一个连接
     * @return 数据库连接的unique_ptr
     */
    std::unique_ptr<PGConnection> GetConnection() {
        std::unique_lock<std::mutex> lock(_mutex);
        
        if (!_initialized) {
            throw std::runtime_error("PostgreSQLPool not initialized");
        }

#ifdef PG_DISABLED
        return std::make_unique<PGConnection>(nullptr);
#else
        _cond.wait(lock, [this] { 
            return _stop || !_connections.empty(); 
        });

        if (_stop) {
            return nullptr;
        }

        auto conn = std::move(_connections.front());
        _connections.pop();
        return conn;
#endif
    }

    /**
     * @brief 归还连接到连接池
     * @param conn 要归还的连接
     */
    void ReturnConnection([[maybe_unused]]std::unique_ptr<PGConnection> conn) {
#ifdef PG_DISABLED
        return;
#else
        if (!conn) {
            return;
        }

        std::lock_guard<std::mutex> lock(_mutex);
        if (_stop) {
            return;
        }

        // 检查连接是否仍然有效
        if (conn->IsValid()) {
            _connections.push(std::move(conn));
            _cond.notify_one();
        } else {
            // 连接无效，创建新连接
            std::cerr << "Connection invalid, creating new one" << std::endl;
            PGconn* newConn = PQconnectdb(_connInfo.c_str());
            if (PQstatus(newConn) == CONNECTION_OK) {
                _connections.push(std::make_unique<PGConnection>(newConn));
                _cond.notify_one();
            } else {
                std::cerr << "Failed to create new connection: " 
                         << PQerrorMessage(newConn) << std::endl;
                PQfinish(newConn);
            }
        }
#endif
    }

    /**
     * @brief 关闭连接池
     */
    void Close() {
        std::lock_guard<std::mutex> lock(_mutex);
        _stop = true;
        _cond.notify_all();
        
        while (!_connections.empty()) {
            _connections.pop();
        }
        _initialized = false;
    }

    /**
     * @brief 获取连接池大小
     */
    size_t GetPoolSize() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _poolSize;
    }

    /**
     * @brief 获取可用连接数
     */
    size_t GetAvailableCount() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _connections.size();
    }

private:
    PostgreSQLPool() : _initialized(false), _stop(false), _poolSize(0) {}
    
    ~PostgreSQLPool() {
        Close();
    }

    mutable std::mutex _mutex;
    std::condition_variable _cond;
    std::queue<std::unique_ptr<PGConnection>> _connections;
    std::string _connInfo;
    size_t _poolSize;
    bool _initialized;
    bool _stop;
};

/**
 * @brief PostgreSQL连接的RAII守卫
 * 自动管理连接的获取和归还
 */
class PGConnectionGuard {
public:
    explicit PGConnectionGuard(PostgreSQLPool& pool) 
        : _pool(pool), _conn(pool.GetConnection()) {}

    ~PGConnectionGuard() {
        if (_conn) {
            _pool.ReturnConnection(std::move(_conn));
        }
    }

    // 禁止拷贝
    PGConnectionGuard(const PGConnectionGuard&) = delete;
    PGConnectionGuard& operator=(const PGConnectionGuard&) = delete;

    PGConnection* operator->() { return _conn.get(); }
    PGConnection& operator*() { return *_conn; }
    PGConnection* Get() { return _conn.get(); }

private:
    PostgreSQLPool& _pool;
    std::unique_ptr<PGConnection> _conn;
};

} // namespace bre

#endif // BRE_POSTGRESQL_POOL_HPP
