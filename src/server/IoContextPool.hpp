#ifndef BRE_IO_CONTEXT_POOL_HPP
#define BRE_IO_CONTEXT_POOL_HPP

#include <boost/asio.hpp>
#include <vector>
#include <memory>
#include <thread>
#include <atomic>

namespace bre {

/**
 * @brief IO Context 池
 * 管理多个io_context，每个运行在独立线程上
 * 使用Round-Robin策略分配io_context
 */
class IoContextPool {
public:
    /**
     * @brief 构造函数
     * @param poolSize IO Context数量（通常设置为CPU核心数）
     */
    explicit IoContextPool(size_t poolSize);

    /**
     * @brief 析构函数
     */
    ~IoContextPool();

    // 禁止拷贝和赋值
    IoContextPool(const IoContextPool&) = delete;
    IoContextPool& operator=(const IoContextPool&) = delete;

    /**
     * @brief 启动所有IO Context
     */
    void Start();

    /**
     * @brief 停止所有IO Context
     */
    void Stop();

    /**
     * @brief 获取一个IO Context（Round-Robin）
     * @return io_context引用
     */
    boost::asio::io_context& GetIoContext();

    /**
     * @brief 获取池大小
     */
    size_t GetPoolSize() const { return _ioContexts.size(); }

private:
    // IO Context列表
    std::vector<std::shared_ptr<boost::asio::io_context>> _ioContexts;
    
    // Work guards防止io_context过早退出
    std::vector<std::shared_ptr<boost::asio::io_context::work>> _workGuards;
    
    // 工作线程
    std::vector<std::thread> _threads;
    
    // Round-Robin计数器
    std::atomic<size_t> _nextIoContext;
    
    // 运行状态
    std::atomic<bool> _running;
};

} // namespace bre

#endif // BRE_IO_CONTEXT_POOL_HPP
