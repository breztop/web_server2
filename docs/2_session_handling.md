# 第2章：Session 处理与连接管理

`Session` 类是本服务器的核心，它代表了一个活跃的 TCP 连接。每个连接都具有独立的生命周期，并负责数据的读写、协议解析及业务处理。

## 1. 生命周期的守护者：`std::enable_shared_from_this`

在异步编程中，最经典的问题是：**当 IO 回调函数执行时，持有这个连接的对象是否还存活？**

```cpp
class Session : public std::enable_shared_from_this<Session> {
    // ...
};
```

通过继承 `enable_shared_from_this` 并使用 `shared_from_this()`，我们可以在异步回调中通过 `self = shared_from_this()` 延长对象的生命周期，直到回调执行完毕。这种机制避免了回调引用已销毁对象的风险（即“僵尸对象”问题）。

## 2. 非阻塞读写循环

Session 的核心工作模式是一个闭环的异步 IO 流程：
1. **DoRead**: 调用 `async_read_some`。
2. **HandleRead**: 收到数据后，送入 `HttpRequest` 解析。
3. **HandleRequest**: 解析完成后，准备 `HttpResponse` 内容。
4. **DoWrite**: 调用 `async_write` 将响应发回。
5. **Repeat**: 如果是 HTTP Keep-Alive，则回到 `DoRead` 继续等待下一个请求。

```cpp
void Session::DoRead() {
    auto self = shared_from_this();
    _socket.async_read_some(
        _readBuffer.prepare(1024),
        [this, self](boost::system::error_code ec, std::size_t bytes_transferred) {
            if (!ec) {
                // 处理数据并决定下一步
            } else {
                Stop(); // 出错或对端关闭，清理资源
            }
        });
}
```

## 3. 连接自动回收：超时管理

为了防止不活跃连接占用服务器资源，我们使用了 `boost::asio::steady_timer`。

* **初始化**: 为每个 Session 创建一个定时器。
* **重置**: 每当有数据收发（读/写完成）时，立即重置定时器。
* **触发**: 如果在设定的时间内（如 60 秒）没有任何 IO 活动，定时器回调将执行并主动关闭 Socket。

```cpp
void Session::StartTimer() {
    _timer.expires_after(_timeoutDuration);
    _timer.async_wait([this, self = shared_from_this()](boost::system::error_code ec) {
        if (!ec) {
            Stop(); // 超时关闭连接
        }
    });
}
```

这种机制在高并发场景下对于防范慢连接攻击（Slowloris）非常有效。
