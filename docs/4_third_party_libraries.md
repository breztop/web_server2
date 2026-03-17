# 第4章：三方库的深度集成

现代 C++ 开发离不开成熟的第三方库。本项目通过合理的库集成，实现了高性能的网络通信和稳定可靠的数据库访问。

## 1. Boost 库家族

**Boost** 是 C++ 界的“准标准库”，本项目大量使用了以下组件：
* **Asio**: 提供跨平台的异步网络 IO 框架，是整个服务器的基础。
* **System**: 用于处理底层操作系统错误码（`error_code`）。
* **Log**: 用于记录服务器运行状态、请求日志及错误排查。

## 2. PostgreSQL 数据库集成：libpqxx

为了支持用户登录、注册等动态功能，我们集成了 PostgreSQL。

### libpqxx
`libpqxx` 是 PostgreSQL 官方推荐的 C++ 客户端。
* **优势**: 强类型检查、自动事务管理（RAII 风格）。
* **连接池 (PostgrePool)**: 为了在高并发下复用数据库连接，本项目封装了 `PostgrePool` 类。它能够管理一组活跃连接，避免了频繁建立 TCP 连接到数据库的性能损耗。

```cpp
// 从池中获取连接
auto conn = _dbPool->GetConnection();
// 执行 SQL
pqxx::work w(*conn);
pqxx::result r = w.exec("SELECT * FROM users");
```

## 3. 性能压测：Google Benchmark

对于一个 Web 服务器，了解其性能上限至关重要。

我们使用了 **Google Benchmark** 来进行压力测试：
* **测试项**: 包含内存分配、HTTP 协议解析速度、Buffer 操作效率等。
* **数据驱动**: 通过 Benchmark 的输出，我们可以直观地看到代码修改对性能带来的影响（提升或下降）。

在 `tests/benchmark.cpp` 中可以找到相关实现：
```cpp
static void BM_HttpParse(benchmark::State& state) {
    for (auto _ : state) {
        // 执行解析逻辑
    }
}
BENCHMARK(BM_HttpParse);
```

## 4. 自用工具库：breutil

本项目还引入了一个小型工具库 `breutil`，封装了开发中常用的功能：
* **Buffer**: 自动扩容的字节缓冲区，支持高效的预分配和读写分离。
* **ThreadPool**: 基于任务队列的通用线程池，用于处理 CPU 密集型任务。
* **Net**: 包含上文提到的 `io_context_pool` 实现。

这些库的有机组合，使得服务器既能保持底层的性能，又能获得高层开发的便利性。
