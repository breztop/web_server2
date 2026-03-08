#pragma once


#include <breutil/singleton.hpp>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>

namespace pqxx {
class connection;
}

class PostgrePool : public bre::Singleton<PostgrePool> {
    friend class bre::Singleton<PostgrePool>;

public:
    class ConnectionGuard {
    public:
        ConnectionGuard() = default;
        ConnectionGuard(PostgrePool* pool, std::shared_ptr<pqxx::connection> connection);
        ConnectionGuard(const ConnectionGuard&) = delete;
        ConnectionGuard& operator=(const ConnectionGuard&) = delete;
        ConnectionGuard(ConnectionGuard&& other) noexcept;
        ConnectionGuard& operator=(ConnectionGuard&& other) noexcept;
        ~ConnectionGuard();

        pqxx::connection& operator*() const;
        pqxx::connection* operator->() const;
        explicit operator bool() const;

    private:
        void Release();

        PostgrePool* _pool{nullptr};
        std::shared_ptr<pqxx::connection> _connection;
    };

public:
    void Init(const std::string& connInfo, size_t poolSize);
    ConnectionGuard Acquire();
    void Close();
    bool IsInitialized() const;
    size_t AvailableCount() const;

    ~PostgrePool();

private:
    PostgrePool();
    void Return(std::shared_ptr<pqxx::connection> connection);

    mutable std::mutex _mutex;
    std::condition_variable _condition;
    std::queue<std::shared_ptr<pqxx::connection>> _pool;
    std::string _connInfo;
    bool _initialized{false};
    bool _closing{false};
    size_t _poolSize{0};
};