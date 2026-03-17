# 第6章：CI 自动化处理流程

持续集成（CI）是现代软件开发不可或缺的一部分。本项目通过 GitHub Actions 实现了全自动化的构建与测试流水线。

## 1. 为什么要使用 CI？

* **代码质量保障**: 每一次提交代码，都会在干净的环境中重新编译，确保代码不会因为本地环境差异而失效。
* **跨平台验证**: 自动在 Linux, Windows, macOS 上编译，第一时间发现跨平台兼容性问题。
* **回归测试**: 自动运行单元测试，确保新功能没有破坏旧功能。

## 2. GitHub Actions 配置文件

流水线定义在 `.github/workflows/cmake-multi-platform.yml` 中。

### 构建矩阵 (Build Matrix)
通过定义 `strategy/matrix`，我们可以一次性启动多个虚拟环境：
```yaml
strategy:
  matrix:
    os: [ubuntu-latest, windows-latest, macos-latest]
```

## 3. 依赖环境的自动化装载

在 CI 环境中，我们必须手动安装项目所需的依赖库。

### Linux (Ubuntu):
使用 `apt-get` 快速安装。
```yaml
- name: Install Boost (Linux)
  run: |
    sudo apt-get update
    sudo apt-get install -y libboost-all-dev libpq-dev libpqxx-dev libbenchmark-dev
```

### macOS:
使用 `brew` 进行安装。

### Windows:
通过 `MarkusJx/install-boost` 这个现成的 Action 来安装 Boost 库。

## 4. 构建与测试流程

流水线主要由以下步骤组成：
1. **Checkout**: 拉取最新代码。
2. **Install Deps**: 安装上述所有依赖。
3. **Configure**: 运行 `cmake -B build` 生成构建系统。
4. **Build**: 运行 `cmake --build build` 执行编译。
5. **Test**: 运行 `ctest` 自动化测试。

## 5. 缓存优化

为了加快构建速度，我们使用了 `actions/cache`：
```yaml
- name: Cache CMake build
  uses: actions/cache@v4
  with:
    path: build
    key: ${{ runner.os }}-cmake-${{ hashFiles('**/CMakeLists.txt') }}
```
它能将上一次编译的中间结果保存下来，下次构建时只需增量编译，显著减少了等待时间。

---

至此，我们的 Web 服务器教学系列文档已全部完成。希望这些文档能帮助你深入理解高性能 C++ 开发的精髓！
