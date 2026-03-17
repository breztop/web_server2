#include "postgre_pool.hpp"

#include <stdexcept>

#ifdef HAVE_POSTGRESQL
#include <pqxx/pqxx>
#endif  // HAVE_POSTGRESQL

PostgrePool::PostgrePool() = default;

PostgrePool::~PostgrePool() { Close(); }

PostgrePool::ConnectionGuard::ConnectionGuard(PostgrePool* pool,
                                              std::shared_ptr<pqxx::connection> connection)
    : _pool(pool)
    , _connection(std::move(connection)) {}

PostgrePool::ConnectionGuard::ConnectionGuard(ConnectionGuard&& other) noexcept
	: _pool(other._pool), _connection(std::move(other._connection)) {
	other._pool = nullptr;
}

PostgrePool::ConnectionGuard& PostgrePool::ConnectionGuard::operator=(ConnectionGuard&& other) noexcept {
	if (this == &other) {
		return *this;
	}

	Release();
	_pool = other._pool;
	_connection = std::move(other._connection);
	other._pool = nullptr;
	return *this;
}

PostgrePool::ConnectionGuard::~ConnectionGuard() { Release(); }

pqxx::connection& PostgrePool::ConnectionGuard::operator*() const { return *_connection; }

pqxx::connection* PostgrePool::ConnectionGuard::operator->() const { return _connection.get(); }

PostgrePool::ConnectionGuard::operator bool() const {
	return _pool != nullptr && static_cast<bool>(_connection);
}

void PostgrePool::ConnectionGuard::Release() {
	if (_pool != nullptr && _connection != nullptr) {
		_pool->Return(std::move(_connection));
	}
	_pool = nullptr;
}

void PostgrePool::Init(const std::string& connInfo, size_t poolSize) {
	if (poolSize == 0) {
		throw std::invalid_argument("database pool size must be greater than 0");
	}

	std::lock_guard<std::mutex> lock(_mutex);

	while (!_pool.empty()) {
		_pool.pop();
	}

	_connInfo = connInfo;
	_poolSize = poolSize;
	_closing = false;

	for (size_t index = 0; index < _poolSize; ++index) {
		auto connection = std::make_shared<pqxx::connection>(_connInfo);
		if (!connection->is_open()) {
			throw std::runtime_error("failed to open database connection");
		}
		_pool.push(std::move(connection));
	}

	_initialized = true;
}

PostgrePool::ConnectionGuard PostgrePool::Acquire() {
	std::unique_lock<std::mutex> lock(_mutex);

	if (!_initialized) {
		throw std::runtime_error("database pool is not initialized");
	}

	_condition.wait(lock, [this]() {
		return _closing || !_pool.empty();
	});

	if (_closing) {
		throw std::runtime_error("database pool is closing");
	}

	auto connection = std::move(_pool.front());
	_pool.pop();
	return ConnectionGuard(this, std::move(connection));
}

void PostgrePool::Close() {
	std::lock_guard<std::mutex> lock(_mutex);
	if (!_initialized && !_closing) {
		return;
	}

	_closing = true;
	_initialized = false;

	while (!_pool.empty()) {
		_pool.pop();
	}

	_condition.notify_all();
}

bool PostgrePool::IsInitialized() const {
	std::lock_guard<std::mutex> lock(_mutex);
	return _initialized && !_closing;
}

size_t PostgrePool::AvailableCount() const {
	std::lock_guard<std::mutex> lock(_mutex);
	return _pool.size();
}

void PostgrePool::Return(std::shared_ptr<pqxx::connection> connection) {
	std::lock_guard<std::mutex> lock(_mutex);
	if (!_initialized || _closing || connection == nullptr) {
		return;
	}

	if (!connection->is_open()) {
		connection = std::make_shared<pqxx::connection>(_connInfo);
	}

	_pool.push(std::move(connection));
	_condition.notify_one();
}
