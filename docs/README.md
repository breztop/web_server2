# C++ 高性能 Web 服务器教学系列

本系列文档通过本项目（`web_server2`）的实战案例，深入讲解 C++ 高性能网络编程、协议解析、数据库集成及工程化实践。文档按照技术层次由浅入深排列。

## 课程导航

### 1. [Asio Server 核心架构](./1_asio_server.md)
* Asio 的事件循环与线程模型
* 多线程 IO 池化的实现 (One-Loop-Per-Thread)
* 异步接收 (Acceptor) 逻辑

### 2. [Session 处理与连接管理](./2_session_handling.md)
* 异步编程中的生命周期管理 (`std::enable_shared_from_this`)
* 异步读写 (Read/Write) 的非阻塞循环
* 自动连接回收与超时定时器 (`steady_timer`)

### 3. [HTTP 协议解析与响应](./3_http_handling.md)
* 有限状态机 (FSM) 解析 HTTP 请求
* 响应状态码、头信息与 Body 的封装
* 静态资源访问与 API 路由基础

### 4. [三方库的深度集成](./4_third_party_libraries.md)
* **Boost.Asio**: 网络通信核心库
* **libpqxx (PostgreSQL)**: 高性能数据库连接池
* **Google Benchmark**: 性能压测与吞吐量分析
* **breutil**: 本项目核心工具组件库

### 5. [CMake 工程化构建实践](./5_cmake_usage.md)
* 模块化 CMake 配置 (.cmake 拆分)
* 跨平台 (Linux/Windows/macOS) 的环境适配
* 依赖库的查找、链接与版本管理

### 6. [CI 自动化处理流程](./6_ci_handling.md)
* GitHub Actions 自动化流水线
* 多平台构建矩阵 (Build Matrix)
* CI 环境下的依赖自动装载与单元测试

---

## 快速上手

建议先通过阅读根目录下的 [QUICKSTART.md](../QUICKSTART.md) 了解如何编译运行项目，然后再深入本系列技术文档进行学习。
