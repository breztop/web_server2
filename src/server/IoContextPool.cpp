#include "IoContextPool.hpp"
#include <iostream>

namespace bre {

IoContextPool::IoContextPool(size_t poolSize)
    : _nextIoContext(0), _running(false) {
    
    if (poolSize == 0) {
        poolSize = std::thread::hardware_concurrency();
        if (poolSize == 0) {
            poolSize = 2; // 默认最小值
        }
    }

    // 创建io_context
    for (size_t i = 0; i < poolSize; ++i) {
        auto ioContext = std::make_shared<boost::asio::io_context>();
        _ioContexts.push_back(ioContext);
        
        // 创建work guard防止io_context过早退出
        auto work = std::make_shared<boost::asio::io_context::work>(*ioContext);
        _workGuards.push_back(work);
    }

    std::cout << "IoContextPool created with " << poolSize << " contexts" << std::endl;
}

IoContextPool::~IoContextPool() {
    Stop();
}

void IoContextPool::Start() {
    if (_running.exchange(true)) {
        return; // 已经运行
    }

    // 为每个io_context创建线程
    for (auto& ioContext : _ioContexts) {
        _threads.emplace_back([ioContext]() {
            try {
                ioContext->run();
            } catch (const std::exception& e) {
                std::cerr << "IoContext thread exception: " << e.what() << std::endl;
            }
        });
    }

    std::cout << "IoContextPool started with " << _threads.size() << " threads" << std::endl;
}

void IoContextPool::Stop() {
    if (!_running.exchange(false)) {
        return; // 未运行
    }

    // 移除work guards，允许io_context退出
    _workGuards.clear();

    // 停止所有io_context
    for (auto& ioContext : _ioContexts) {
        ioContext->stop();
    }

    // 等待所有线程完成
    for (auto& thread : _threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    _threads.clear();
    std::cout << "IoContextPool stopped" << std::endl;
}

boost::asio::io_context& IoContextPool::GetIoContext() {
    // Round-Robin策略
    size_t index = _nextIoContext.fetch_add(1) % _ioContexts.size();
    return *_ioContexts[index];
}

} // namespace bre
