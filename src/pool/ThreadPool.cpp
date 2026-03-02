#include "ThreadPool.hpp"

namespace bre {

ThreadPool::ThreadPool(size_t threadCount)
    : _stop(false), _activeThreads(0) {
    
    if (threadCount == 0) {
        threadCount = 1;
    }

    _workers.reserve(threadCount);
    for (size_t i = 0; i < threadCount; ++i) {
        _workers.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                
                {
                    std::unique_lock<std::mutex> lock(_queueMutex);
                    _condition.wait(lock, [this] {
                        return _stop || !_tasks.empty();
                    });

                    if (_stop && _tasks.empty()) {
                        return;
                    }

                    task = std::move(_tasks.front());
                    _tasks.pop();
                }

                ++_activeThreads;
                task();
                --_activeThreads;
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(_queueMutex);
        _stop = true;
    }
    _condition.notify_all();
    
    for (auto& worker : _workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

size_t ThreadPool::GetThreadCount() const {
    return _workers.size();
}

size_t ThreadPool::GetQueueSize() const {
    std::unique_lock<std::mutex> lock(_queueMutex);
    return _tasks.size();
}

size_t ThreadPool::GetActiveThreadCount() const {
    return _activeThreads.load();
}

void ThreadPool::WaitAll() {
    while (true) {
        {
            std::unique_lock<std::mutex> lock(_queueMutex);
            if (_tasks.empty() && _activeThreads == 0) {
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

} // namespace bre
