# 第5章：CMake 工程化构建实践

对于大型 C++ 项目，良好的构建系统是可维护性的关键。本项目采用了模块化的 CMake 配置，以支持跨平台编译并高效管理复杂的依赖项。

## 1. 模块化构建架构

本项目没有将所有逻辑堆砌在根目录的 `CMakeLists.txt` 中，而是将不同的配置逻辑拆分到了 `cmake/` 目录下的多个 `.cmake` 文件中。

### 核心子模块：
* **`link_boost.cmake`**: 处理 Boost 库的查找 (`find_package`) 和链接。
* **`link_libpqcxx.cmake`**: 处理 PostgreSQL 客户端库的依赖。
* **`link_benchmark.cmake`**: 只有在需要测试时才会启用的 Benchmark 链接逻辑。
* **`compile_option.cmake`**: 统一设置编译器标志（如开启 C++20、警告级别等）。

通过 `include(cmake/xxx.cmake)`，主配置文件保持了极高的可读性。

## 2. 跨平台支持

本项目支持在 Linux, Windows (MSVC), 和 macOS 上编译运行。

### CMake 策略：
1. **find_package**: 尽量使用标准的方法查找系统库。
2. **条件编译**: 针对不同操作系统设置特定的库路径或宏定义。
   ```cmake
   if(WIN32)
       # Windows 专用设置
   elseif(APPLE)
       # macOS 专用设置
   endif()
   ```
3. **输出路径统一**: 设置 `EXECUTABLE_OUTPUT_PATH` 确保无论在哪里构建，最终的可执行文件都会生成在 `run/` 目录下。

## 3. 依赖管理技巧

在 C++ 中管理依赖通常是一件头疼的事。本项目采用以下策略：
* **系统包优先**: 在 Linux/macOS 上，建议通过 `apt` 或 `brew` 预先安装好依赖库。
* **显式链接**: 使用 `target_link_libraries` 而不是全局的 `link_libraries`，以明确每个目标的依赖关系，减少编译污染。

## 4. 编译建议

为了提高开发效率，我们建议使用以下流程进行构建：
```bash
# 1. 创建并进入构建目录
mkdir build && cd build
# 2. 生成构建文件
cmake ..
# 3. 编译（-j8 表示使用 8 个线程）
cmake --build . -j8
```

通过这种标准的现代 CMake 流程，开发者可以轻松地在不同的 IDE（如 VSCode, Visual Studio, CLion）中导入并进行开发。
