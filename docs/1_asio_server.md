# 第1章：Asio Server 核心架构

在高性能 Web 服务器开发中，网络 IO 的处理效率直接决定了系统的并发上限。本项目基于 **Boost.Asio** 库，采用经典的异步 IO (Proactor) 模型。

## 1. Asio 的核心组件

* **io_context**: Asio 的事件循环核心。它负责监听 IO 事件（如读就绪、写完成、新连接）并分发给相应的回调函数。
* **acceptor**: 专门用于监听特定端口并接受新 TCP 连接的组件。

## 2. 并发模型：IO 池化

本项目采用了 **One-Loop-Per-Thread** 模型。相比于所有线程竞争同一个 `io_context`，这种模型减少了内部锁的争用，提升了高并发下的扩展性。

### 实现方案：`asio_io_context_pool`
在 `WebServer.hpp` 中：
```cpp
// 线程池（用于 CPU 密集型任务）
std::unique_ptr<ThreadPool> _threadPool;

// IO 池化管理
// 在 breutil/net/asio_io_context_pool.hpp 中定义
```
服务器启动时会根据配置创建多个线程，每个线程运行一个独立的 `io_context::run()`。当新连接建立时，服务器会以 Round-Robin（轮询）的方式将其分配给池中的一个 `io_context` 处理。

## 3. 异步接收逻辑

异步服务器的启动流程如下：
1. **StartAccept**: 预注册一个监听动作。
2. **HandleAccept**: 当有新连接进来时，Asio 会触发此回调。

```cpp
void WebServer::StartAccept() {
    _acceptor->async_accept(
        [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
            HandleAccept(ec, std::move(socket));
        });
}
```

在 `HandleAccept` 中，我们完成以下关键动作：
1. **分配 Session**: 为新 socket 创建一个 `Session` 对象。
2. **分配 IO 线程**: 从 IO 池中挑选一个 `io_context` 绑定给该 Session。
3. **继续监听**: 立即调用 `StartAccept()` 准备接受下一个连接。

这种“先注册、后回调、再递归注册”的模式，构成了 Asio 异步驱动的基础。
