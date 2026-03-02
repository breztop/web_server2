#ifndef BRE_THREAD_POOL_HPP
#define BRE_THREAD_POOL_HPP

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <stdexcept>
#include <atomic>

namespace bre {

/**
 * @brief 高性能线程池
 * 支持任务队列管理和工作线程调度
 */
class ThreadPool {
public:
    /**
     * @brief 构造函数
     * @param threadCount 线程数量，默认为硬件并发数
     */
    explicit ThreadPool(size_t threadCount = std::thread::hardware_concurrency());

    /**
     * @brief 析构函数
     * 等待所有任务完成并关闭线程池
     */
    ~ThreadPool();

    // 禁止拷贝和赋值
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    /**
     * @brief 提交任务到线程池
     * @param f 任务函数
     * @param args 函数参数
     * @return future对象，用于获取任务结果
     */
    template<typename F, typename... Args>
    auto Submit(F&& f, Args&&... args) 
        -> std::future<typename std::invoke_result_t<F, Args...>> {
        
        using return_type = typename std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> result = task->get_future();
        
        {
            std::unique_lock<std::mutex> lock(_queueMutex);
            if (_stop) {
                throw std::runtime_error("Cannot submit task to stopped ThreadPool");
            }
            _tasks.emplace([task]() { (*task)(); });
        }
        
        _condition.notify_one();
        return result;
    }

    /**
     * @brief 获取线程池中的线程数量
     */
    size_t GetThreadCount() const;

    /**
     * @brief 获取等待队列中的任务数量
     */
    size_t GetQueueSize() const;

    /**
     * @brief 获取活跃线程数量
     */
    size_t GetActiveThreadCount() const;

    /**
     * @brief 等待所有任务完成
     */
    void WaitAll();

private:
    std::vector<std::thread> _workers;              // 工作线程
    std::queue<std::function<void()>> _tasks;       // 任务队列
    mutable std::mutex _queueMutex;                 // 队列互斥锁
    std::condition_variable _condition;             // 条件变量
    std::atomic<bool> _stop;                        // 停止标志
    std::atomic<size_t> _activeThreads;             // 活跃线程计数
};

} // namespace bre

#endif // BRE_THREAD_POOL_HPP
