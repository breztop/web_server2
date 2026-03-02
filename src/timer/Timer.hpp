#ifndef BRE_TIMER_HPP
#define BRE_TIMER_HPP

#include <boost/asio.hpp>
#include <chrono>
#include <functional>
#include <memory>
#include <map>
#include <mutex>

namespace bre {

using namespace std::chrono;

/**
 * @brief 基于ASIO的定时器管理器
 * 支持一次性定时器和周期性定时器
 */
class Timer {
public:
    using TimePoint = steady_clock::time_point;
    using Duration = steady_clock::duration;
    using TimerCallback = std::function<void()>;
    using TimerId = uint64_t;

    /**
     * @brief 构造函数
     * @param ioContext ASIO IO上下文引用
     */
    explicit Timer(boost::asio::io_context& ioContext) 
        : _ioContext(ioContext), _nextTimerId(1) {}

    ~Timer() {
        CancelAll();
    }

    // 禁止拷贝
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;

    /**
     * @brief 添加一次性定时器
     * @param delay 延迟时间
     * @param callback 回调函数
     * @return 定时器ID
     */
    TimerId AddTimer(Duration delay, TimerCallback callback) {
        std::lock_guard<std::mutex> lock(_mutex);
        
        TimerId id = _nextTimerId++;
        auto timer = std::make_shared<boost::asio::steady_timer>(_ioContext, delay);
        
        timer->async_wait([this, id, callback](const boost::system::error_code& ec) {
            if (!ec) {
                callback();
                RemoveTimer(id);
            }
        });

        _timers[id] = timer;
        return id;
    }

    /**
     * @brief 添加周期性定时器
     * @param interval 时间间隔
     * @param callback 回调函数
     * @return 定时器ID
     */
    TimerId AddRepeatingTimer(Duration interval, TimerCallback callback) {
        std::lock_guard<std::mutex> lock(_mutex);
        
        TimerId id = _nextTimerId++;
        auto timer = std::make_shared<boost::asio::steady_timer>(_ioContext, interval);
        
        _startRepeatingTimer(id, timer, interval, callback);
        _timers[id] = timer;
        
        return id;
    }

    /**
     * @brief 取消指定定时器
     * @param id 定时器ID
     * @return 成功返回true，否则返回false
     */
    bool CancelTimer(TimerId id) {
        std::lock_guard<std::mutex> lock(_mutex);
        return _cancelTimer(id);
    }

    /**
     * @brief 取消所有定时器
     */
    void CancelAll() {
        std::lock_guard<std::mutex> lock(_mutex);
        for (auto& [id, timer] : _timers) {
            if (timer) {
                boost::system::error_code ec;
                timer->cancel(ec);
            }
        }
        _timers.clear();
    }

    /**
     * @brief 获取当前活跃的定时器数量
     */
    size_t GetTimerCount() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _timers.size();
    }

private:
    void _startRepeatingTimer(TimerId id, 
                             std::shared_ptr<boost::asio::steady_timer> timer,
                             Duration interval,
                             TimerCallback callback) {
        timer->async_wait([this, id, timer, interval, callback]
                         (const boost::system::error_code& ec) {
            if (!ec) {
                callback();
                
                // 重新设置定时器
                timer->expires_at(timer->expiry() + interval);
                _startRepeatingTimer(id, timer, interval, callback);
            }
        });
    }

    bool _cancelTimer(TimerId id) {
        auto it = _timers.find(id);
        if (it != _timers.end()) {
            if (it->second) {
                boost::system::error_code ec;
                it->second->cancel(ec);
            }
            _timers.erase(it);
            return true;
        }
        return false;
    }

    void RemoveTimer(TimerId id) {
        std::lock_guard<std::mutex> lock(_mutex);
        _timers.erase(id);
    }

    boost::asio::io_context& _ioContext;
    mutable std::mutex _mutex;
    std::map<TimerId, std::shared_ptr<boost::asio::steady_timer>> _timers;
    TimerId _nextTimerId;
};

/**
 * @brief 连接超时管理器
 * 管理连接的空闲超时
 */
class ConnectionTimer {
public:
    explicit ConnectionTimer(boost::asio::io_context& ioContext, Duration timeout)
        : _timer(ioContext), _timeout(timeout) {}

    /**
     * @brief 启动或重置超时定时器
     * @param callback 超时回调
     */
    void Start(std::function<void()> callback) {
        _callback = callback;
        _timer.expires_from_now(_timeout);
        _timer.async_wait([this](const boost::system::error_code& ec) {
            if (!ec && _callback) {
                _callback();
            }
        });
    }

    /**
     * @brief 重置定时器
     */
    void Reset() {
        boost::system::error_code ec;
        _timer.cancel(ec);
        if (_callback) {
            Start(_callback);
        }
    }

    /**
     * @brief 取消定时器
     */
    void Cancel() {
        boost::system::error_code ec;
        _timer.cancel(ec);
    }

    /**
     * @brief 设置超时时间
     */
    void SetTimeout(Duration timeout) {
        _timeout = timeout;
    }

private:
    boost::asio::steady_timer _timer;
    Duration _timeout;
    std::function<void()> _callback;
};

} // namespace bre

#endif // BRE_TIMER_HPP
