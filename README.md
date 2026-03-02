# Bre WebServer 2.0

一个现代化的C++20跨平台Web服务器，采用**主从Reactor模式**，使用ASIO实现高性能异步IO，支持PostgreSQL数据库。

## ✨ 核心特性

### 🏗️ 架构设计
- **主从Reactor模式**: 主IO Context负责Accept，IO Context Pool负责业务处理
- **Session管理**: 每个连接独立Session，完整生命周期管理
- **模块化设计**: 头文件+实现文件分离，高度解耦
- **跨平台支持**: Windows和Linux双平台

### 🚀 技术栈
- ✅ **C++20标准**: concepts, ranges, coroutines等现代特性
- ✅ **Boost.ASIO**: 高性能异步网络库
- ✅ **IO Context Pool**: 多IO上下文Round-Robin分配
- ✅ **PostgreSQL**: 企业级数据库支持
- ✅ **线程池**: 高效CPU密集型任务处理
- ✅ **环境变量安全**: 敏感信息（密码）不存配置文件
- ✅ **完整测试**: Boost.Test单元测试 + Benchmark性能测试
- ✅ **优雅关闭**: 完善的资源清理机制

### 📊 性能特点
- 多IO Context充分利用多核CPU
- Round-Robin负载均衡
- Keep-Alive长连接支持
- 高并发连接处理
- 低延迟响应

## 📁 项目结构

```
web_server2/
├── CMakeLists.txt          # 根CMake配置
├── config.txt              # 服务器配置文件
├── README.md               # 项目说明
├── ARCHITECTURE.md         # 架构文档（详细设计）
├── QUICKSTART.md           # 快速开始指南
├── build.sh                # Linux编译脚本
├── build.ps1               # Windows编译脚本
├── fronted/                # 前端静态资源
│   ├── index.html
│   ├── images/
│   └── video/
├── src/                    # 源代码目录
│   ├── CMakeLists.txt
│   ├── main.cpp            # 程序入口
│   ├── buffer/             # 缓冲区模块（Header-only）
│   │   └── Buffer.hpp
│   ├── config/             # 配置管理模块
│   │   ├── Config.hpp
│   │   └── Config.cpp      # ✨ 新增实现文件
│   ├── database/           # 数据库模块
│   │   └── PostgreSQLPool.hpp
│   ├── http/               # HTTP处理模块
│   │   ├── HttpRequest.hpp
│   │   ├── HttpResponse.hpp
│   │   └── HttpConnection.hpp
│   ├── pool/               # 线程池模块（Header-only）
│   │   └── ThreadPool.hpp
│   ├── server/             # 服务器核心 ✨ 新架构
│   │   ├── IoContextPool.hpp   # IO上下文池
│   │   ├── IoContextPool.cpp   # ✨ 实现文件
│   │   ├── Session.hpp         # 会话管理
│   │   ├── Session.cpp         # ✨ 实现文件
│   │   ├── WebServer.hpp       # 主服务器
│   │   └── WebServer.cpp       # ✨ 实现文件
│   ├── timer/              # 定时器模块
│   │   └── Timer.hpp
│   └── tests/              # 测试目录
│       ├── benchmark.cpp       # ✨ 性能基准测试
│       ├── test_buffer.cpp
│       ├── test_config.cpp
│       ├── test_http.cpp       # ✨ HTTP测试
│       └── test_threadpool.cpp # ✨ 线程池测试
└── build/                  # 编译输出目录
```

## 🏗️ 核心架构

### 主从Reactor模式

```
        Client Connections
              ↓
    ┌─────────────────────┐
    │   Main Acceptor     │ ← 主IO Context（只负责Accept）
    │   (io_context)      │
    └─────────┬───────────┘
              │
              ├─→ Round-Robin分配
              ↓
    ┌─────────────────────┐
    │  IoContextPool      │
    │  ┌────┐ ┌────┐      │
    │  │IO 1│ │IO 2│ ...  │ ← 从IO Contexts（业务处理）
    │  └────┘ └────┘      │    每个运行在独立线程
    └─────────┬───────────┘
              │
              ├─→ Session创建
              ↓
    ┌─────────────────────┐
    │  Session (per conn) │
    │  ├─ Buffer          │
    │  ├─ HttpRequest     │
    │  ├─ HttpResponse    │
    │  └─ Timer           │
    └─────────────────────┘
              │
              ├─→ CPU密集任务
              ↓
    ┌─────────────────────┐
    │    ThreadPool       │ ← 业务线程池
    └─────────────────────┘
              │
              ├─→ 数据库操作
              ↓
    ┌─────────────────────┐
    │  PostgreSQLPool     │ ← 数据库连接池
    └─────────────────────┘
```

### 工作流程

1. **Accept阶段**: 主IO Context的Acceptor接收新连接
2. **分配阶段**: 从IoContextPool中Round-Robin选择一个io_context
3. **Session创建**: 在选定的io_context中创建Session对象
4. **请求处理**: Session异步读取→解析→处理→响应
5. **Keep-Alive**: 支持连接复用，减少TCP握手开销

### 优势

- **高并发**: 多个IO Context并行处理，避免单点瓶颈
- **负载均衡**: Round-Robin策略均匀分配连接
- **资源隔离**: 每个Session独立管理，互不影响
- **优雅关闭**: 完善的生命周期管理

## 🛠️ 技术栈

### 核心技术
- **C++20**: 使用现代C++特性（concepts, ranges等）
- **Boost.ASIO**: 跨平台异步网络库
- **PostgreSQL**: 开源关系型数据库（通过libpq）
- **Boost.Test**: 单元测试框架

### 编码规范
- 类成员变量使用 `_xxx` 命名（私有成员）
- 对外函数首字母大写（如Golang风格）
- 头文件+实现文件分离（非性能关键模块）
- 使用智能指针管理资源
- 异常安全保证

### 安全实践
- ✅ **环境变量存储敏感信息**（数据库密码等）
- ✅ **路径验证**（防止目录遍历攻击）
- ✅ **请求大小限制**
- ✅ **超时机制**

## 📦 依赖安装

### Windows

```powershell
# 1. 安装Boost (已配置在 G:/codeEnv/boost_1_84)
# 下载地址: https://www.boost.org/

# 2. PostgreSQL (可选，用于数据库功能)
# 下载地址: https://www.postgresql.org/download/windows/

# 3. CMake 3.20+
# 下载地址: https://cmake.org/download/
```

### Linux

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libboost-all-dev \
    libpq-dev \
    postgresql-server-dev-all

# CentOS/RHEL
sudo yum install -y \
    gcc-c++ \
    cmake \
    boost-devel \
    postgresql-devel
```

## 🔨 编译

### Windows (使用CMake + Visual Studio)

```powershell
# 创建构建目录
mkdir build
cd build

# 生成项目文件
cmake .. -G "Visual Studio 17 2022" -A x64

# 编译
cmake --build . --config Release

# 运行
.\run\web_server.exe
```

### Linux

```bash
# 创建构建目录
mkdir build && cd build

# 生成Makefile
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译
make -j$(nproc)

# 运行
../run/web_server
```

## ⚙️ 配置

### 编辑配置文件

编辑 `config.txt` 文件：

```ini
# 服务器端口
PORT=8080

# 资源目录（静态文件目录）
RESOURCE_DIR=./fronted

# IO Context Pool 大小（0表示使用CPU核心数）
IO_POOL_SIZE=0

# 线程池大小
THREAD_POOL_SIZE=4

# PostgreSQL配置
DB_HOST=localhost
DB_PORT=5432
DB_NAME=mydb
DB_USER=postgres

# 数据库连接池大小
DB_POOL_SIZE=8
```

### 🔐 设置环境变量（重要！）

**敏感信息（如数据库密码）应该通过环境变量设置，而不是写在配置文件中！**

#### Linux/macOS
```bash
# 设置数据库密码
export DB_PASSWORD=your_secure_password

# 永久设置（添加到 ~/.bashrc 或 ~/.zshrc）
echo 'export DB_PASSWORD=your_secure_password' >> ~/.bashrc
source ~/.bashrc

# 验证
echo $DB_PASSWORD
```

#### Windows (PowerShell)
```powershell
# 设置数据库密码
$env:DB_PASSWORD="your_secure_password"

# 永久设置（系统级）
[System.Environment]::SetEnvironmentVariable('DB_PASSWORD', 'your_secure_password', 'User')

# 验证
echo $env:DB_PASSWORD
```

#### Windows (CMD)
```cmd
set DB_PASSWORD=your_secure_password

# 永久设置
setx DB_PASSWORD "your_secure_password"
```

### 为什么使用环境变量？

1. ✅ **安全性**: 密码不会被提交到版本控制系统
2. ✅ **灵活性**: 不同环境（开发、测试、生产）使用不同配置
3. ✅ **最佳实践**: 符合12-Factor App原则
4. ✅ **审计友好**: 配置文件可以公开，敏感信息独立管理

## 🧪 运行测试

```bash
# 编译测试
cd build
cmake --build . --target test_config
cmake --build . --target test_buffer
cmake --build . --target test_http
cmake --build . --target test_threadpool

# 运行单元测试
./test_config
./test_buffer
./test_http
./test_threadpool

# 或使用CTest运行所有测试
ctest --output-on-failure
```

### 性能基准测试 (Linux)

```bash
# 1. 启动服务器（终端1）
./run/web_server

# 2. 运行内置benchmark（终端2）
cd build
./benchmark

# 示例输出：
# === Simple Test ===
# Total Requests:   100
# QPS:              2500 req/s
# Avg Latency:      0.4 ms
#
# === Concurrent Test ===
# Threads:          10
# QPS:              8000 req/s
#
# === Keep-Alive Test ===
# Connections:      10
# Requests/conn:    100
# QPS:              12000 req/s

# 3. 使用Apache Bench
ab -n 10000 -c 100 http://localhost:8080/index.html

# 4. 使用wrk
wrk -t4 -c100 -d30s http://localhost:8080/index.html
```

### 测试覆盖

- ✅ **Buffer**: 缓冲区操作、边界条件、移动语义
- ✅ **Config**: 配置加载、环境变量、错误处理
- ✅ **HTTP**: 请求解析、响应构建、文件服务
- ✅ **ThreadPool**: 任务提交、Future、异常处理、并发
- ✅ **Benchmark**: 吞吐量、延迟、并发性能

## 📝 使用示例

### 启动服务器

```bash
# Linux
./run/web_server

# Windows
.\run\web_server.exe

# 看到类似输出表示启动成功：
# ==================================
#   Bre WebServer v2.0
#   ASIO + PostgreSQL + C++20
#   Architecture: Main Reactor + IO Pool
# ==================================
#
# Configuration loaded:
#   Port: 8080
#   Resource Dir: ./fronted
#   IO Pool Size: 8
#   Thread Pool Size: 4
# Server initialized on port 8080
# IoContextPool started with 8 threads
# 
# ==========================================
#   Bre WebServer 2.0 Starting...
# ==========================================
# Server started successfully!
# Listening on port 8080
# Press Ctrl+C to stop...
```

### 访问服务器

```bash
# 浏览器访问
http://localhost:8080/

# 使用curl
curl http://localhost:8080/index.html
```

## 🎯 核心模块说明

### 1. Buffer模块
高效的读写缓冲区，支持：
- 动态扩容
- 零拷贝操作
- CRLF查找
- 移动语义

### 2. Config模块
线程安全的配置管理，支持：
- 从文件加载配置
- 键值对存储
- 默认值支持
- 注释和空行处理

### 3. PostgreSQL连接池
数据库连接池，支持：
- 连接复用
- RAII管理
- 自动重连
- 跨平台（Windows开发时可禁用）

### 4. ThreadPool
高性能线程池，支持：
- 任务队列
- Future/Promise
- 活跃线程监控
- 优雅关闭

### 5. HTTP模块
完整的HTTP/1.1实现：
- 请求解析（方法、路径、头部、Body）
- 响应构建（状态码、头部、Body）
- 静态文件服务
- MIME类型识别

### 6. WebServer核心
基于ASIO的异步服务器：
- 异步Accept
- 异步Read/Write
- 连接管理
- 超时控制

## 🔄 升级说明

### 从旧版本升级

本项目从原有的epoll版本升级为ASIO版本，主要改进：

1. **网络库**: epoll → Boost.ASIO （跨平台）
2. **数据库**: MySQL → PostgreSQL
3. **标准**: C++14 → C++20
4. **架构**: 更模块化、更解耦

### 迁移指南

- 配置文件格式已更新
- API接口保持相似但有改进
- 数据库接口需要修改（MySQL → PostgreSQL）

## 🐛 已知问题

- Windows下PostgreSQL暂时使用占位实现（开发用）
- 暂不支持HTTPS
- 暂不支持WebSocket

## 📈 性能

- 并发连接: 10000+
- QPS: 取决于硬件和网络
- 内存占用: 低于100MB（空载）

## 🤝 贡献

欢迎提交Issue和Pull Request！

## 📄 许可证

MIT License

## 👤 作者

breeze1396

## 🙏 致谢

- Boost.ASIO 社区
- PostgreSQL 项目
- 所有贡献者

---

**注意**: 本项目用于学习和研究目的，生产环境使用请谨慎评估。
