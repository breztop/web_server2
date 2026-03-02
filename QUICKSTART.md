# Bre WebServer 2.0 - 快速开始指南

## 📋 前置要求

### Windows
- Visual Studio 2022 或更高版本
- CMake
- Boost （已配置在 `G:/codeEnv/boost_1_84`）

### Linux
- GCC 11+ 或 Clang 14+
- CMake
- Boost （已安装）
- PostgreSQL 开发库 (可选)

## 🚀 快速开始

### 1. 克隆项目

### 2. 配置项目

编辑 `config.txt`:
```ini
PORT=8080
RESOURCE_DIR=./fronted
THREAD_COUNT=8
TIMEOUT=5000
```

### 3. Windows 编译

#### 使用 CMake GUI
1. 打开 CMake GUI
2. Source: `g:\0_cpp\web_server2`
3. Build: `g:\0_cpp\web_server2\build`
4. 点击 Configure → Generate
5. 打开 `build/breWebserver.sln`
6. 编译 Release 版本

#### 使用命令行
```powershell
# 创建构建目录
mkdir build
cd build

# 配置项目
cmake .. -G "Visual Studio 17 2022" -A x64

# 编译
cmake --build . --config Release

# 运行
cd ..
.\run\web_server.exe
```

### 4. Linux 编译

```bash
# 安装依赖
sudo apt-get install -y build-essential cmake libboost-all-dev libpq-dev

# 创建构建目录
mkdir build && cd build

# 配置并编译
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# 运行
cd ..
./run/web_server
```

## 🧪 运行测试

### Windows
```powershell
cd build
cmake --build . --config Release --target test_config
cmake --build . --config Release --target test_buffer

# 运行测试
.\Release\test_config.exe
.\Release\test_buffer.exe
```

### Linux
```bash
cd build
make test_config test_buffer

# 运行测试
./test_config
./test_buffer

# 或使用 CTest
ctest --output-on-failure
```

## 📝 验证安装

### 1. 启动服务器
```bash
# 确保在项目根目录
./run/web_server
```

你应该看到类似输出：
```
==================================
  Bre WebServer v2.0
  Using ASIO + PostgreSQL
==================================
WebServer starting on port 8080...
Resource directory: ./fronted
Thread pool size: 8
```

### 2. 测试服务器

打开浏览器访问:
- `http://localhost:8080/`
- `http://localhost:8080/index.html`

或使用 curl:
```bash
curl http://localhost:8080/index.html
```

## 🔧 常见问题

### Q1: CMake找不到Boost
**Windows解决方案:**
```cmake
# 在 CMakeLists.txt 中检查或修改 BOOST_ROOT
set(BOOST_ROOT "G:/codeEnv/boost_1_84" CACHE PATH "Boost root directory")
```

**Linux解决方案:**
```bash
sudo apt-get install libboost-all-dev
```

### Q2: 编译错误 - C++20特性不支持
确保使用足够新的编译器:
- GCC 11+
- Clang 14+
- MSVC 2022+

### Q3: 端口被占用
修改 `config.txt` 中的端口:
```ini
PORT=9090
```

### Q4: 找不到资源文件
检查 `RESOURCE_DIR` 配置:
```ini
RESOURCE_DIR=./fronted
```

确保 `fronted` 目录存在且包含 `index.html`。

### Q5: PostgreSQL连接失败
如果不需要数据库功能，保持配置为空:
```ini
DB_CONN_INFO=
```

如果需要，确保PostgreSQL服务运行:
```bash
# Linux
sudo systemctl status postgresql

# 配置连接信息
DB_CONN_INFO=host=localhost port=5432 dbname=mydb user=postgres password=yourpass
```

## 🎯 开发提示

### 添加新的HTTP路由

在 `HttpConnection.hpp` 的 `_handleRequest` 方法中添加:
```cpp
if (path == "/api/users") {
    // 处理API请求
    response = HttpResponse::MakeJsonResponse("{\"status\":\"ok\"}");
}
```

### 使用数据库

```cpp
// 获取连接
auto& pool = PostgreSQLPool::GetInstance();
PGConnectionGuard conn(pool);

// 执行查询 (Linux)
// PGresult* result = PQexec(conn->Get(), "SELECT * FROM users");
```

### 添加中间件

在 `HttpConnection` 中添加请求处理链:
```cpp
void _handleRequest() {
    // 1. 日志记录
    _logRequest();
    
    // 2. 认证检查
    if (!_checkAuth()) {
        response = HttpResponse::MakeErrorResponse(HttpStatus::UNAUTHORIZED);
        return;
    }
    
    // 3. 业务逻辑
    _processRequest();
}
```

## 📊 性能优化建议

1. **调整线程池大小**
   ```ini
   THREAD_COUNT=16  # 根据CPU核心数调整
   ```

2. **启用编译优化**
   ```bash
   cmake .. -DCMAKE_BUILD_TYPE=Release
   ```

3. **调整超时时间**
   ```ini
   TIMEOUT=3000  # 减少空闲连接占用
   ```

## 🔄 从旧版本迁移

### 配置文件迁移
旧配置 → 新配置:
- `PORT:5678` → `PORT=5678`
- `PATH:/resources` → `RESOURCE_DIR=./fronted`
- `CONNPOOLNUM:12` → `DB_POOL_SIZE=12`

### 代码迁移
主要变化:
1. epoll → ASIO
2. MySQL → PostgreSQL
3. 回调风格 → 异步async_xxx

## 📚 更多资源

- [Boost.ASIO 文档](https://www.boost.org/doc/libs/release/doc/html/boost_asio.html)
- [PostgreSQL C API](https://www.postgresql.org/docs/current/libpq.html)
- [C++20 新特性](https://en.cppreference.com/w/cpp/20)

## 💡 下一步

1. ✅ 熟悉项目结构
2. ✅ 运行示例和测试
3. 🔄 添加自定义功能
4. 🔄 性能测试和优化
5. 🔄 部署到生产环境

---

如有问题，请查看 [README.md](README.md) 或提交 Issue。
