cmake_minimum_required(VERSION 3.20)

# Set policies to avoid Boost-related warnings
if(POLICY CMP0144)
    cmake_policy(SET CMP0144 NEW)
endif()
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

project(bre_webserver2_project VERSION 2.0 LANGUAGES CXX)

# C++20标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置输出路径
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/run)

# 存在 cmake/platform.cmake 文件才包含
if (EXISTS "${CMAKE_SOURCE_DIR}/cmake/platform.cmake")
    include(cmake/platform.cmake)
endif()
include(cmake/compile_option.cmake)
include(cmake/generate_build_info.cmake)

include(cmake/link_boost.cmake)
include(cmake/link_libpqcxx.cmake)
include(cmake/link_breutil.cmake)
include(cmake/link_benchmark.cmake)

find_package(Threads REQUIRED)

# 添加子目录
add_subdirectory(src)




