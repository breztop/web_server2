cmake_minimum_required(VERSION 3.20)

# Set policies to avoid Boost-related warnings
if(POLICY CMP0144)
    cmake_policy(SET CMP0144 NEW)
endif()
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

project(breWebserver VERSION 2.0 LANGUAGES CXX)

# C++20标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置输出路径
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/run)

# 编译选项
if(MSVC)
    # /utf-8 使 MSVC 以 UTF-8 解释源码并生成 UTF-8 本地执行环境相关的窄字符串常量，解决 C4819 警告
    add_compile_options(/W4 /permissive- /utf-8)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# 检测平台
if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)  # Windows 7及以上
    set(PLATFORM_WINDOWS ON)
elseif(UNIX)
    set(PLATFORM_LINUX ON)
endif()

# 查找依赖库
find_package(Threads REQUIRED)

# Boost (仅头文件版本的ASIO)
if(WIN32)
    # Windows下需要手动设置Boost路径
    set(BOOST_ROOT "G:/codeEnv/boost_1_84" CACHE PATH "Boost root directory")
    set(Boost_INCLUDE_DIR "${BOOST_ROOT}/include")
    set(Boost_LIBRARY_DIR "${BOOST_ROOT}/lib")
endif()
find_package(Boost REQUIRED COMPONENTS unit_test_framework)

# PostgreSQL
if(PLATFORM_LINUX)
    find_package(PostgreSQL REQUIRED)
endif()

# 包含目录
## Boost 头文件目录
# 如果使用手动指定 BOOST_ROOT，则上面设置的是 Boost_INCLUDE_DIR；find_package 则提供 Boost_INCLUDE_DIRS
# 某些情况下 Boost_INCLUDE_DIRS 可能为空（手动设置时），因此同时兼容两者。
if(Boost_INCLUDE_DIRS)
    include_directories(${Boost_INCLUDE_DIRS})
endif()
if(Boost_INCLUDE_DIR)
    include_directories(${Boost_INCLUDE_DIR})
endif()
include_directories(${CMAKE_SOURCE_DIR}/src)

# 添加子目录
add_subdirectory(src)



