cmake_minimum_required(VERSION 3.20)
project(bre_webserver2_project VERSION 2.0 LANGUAGES CXX)

# Set policies to avoid Boost-related warnings
if(POLICY CMP0144)
    cmake_policy(SET CMP0144 NEW)
endif()
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()





### setting
set(NEED_POSTGRESQL  OFF CACHE BOOL "是否启用PostgreSQL支持")
set(BUILD_TESTING OFF CACHE BOOL "是否构建测试")
set(BUILD_BENCHMARK OFF CACHE BOOL "是否构建benchmark")

### ====================================================





# C++20标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置输出路径
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/run)



if(BUILD_TESTING)
    enable_testing()
    include(CTest)
endif()

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


# print config
message(STATUS "WebServer config")
message(STATUS "  CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
message(STATUS "  CMAKE_CXX_STANDARD: ${CMAKE_CXX_STANDARD}")
message(STATUS "  NEED_POSTGRESQL: ${NEED_POSTGRESQL}")
message(STATUS "  BUILD_TESTING: ${BUILD_TESTING}")
message(STATUS "  BUILD_BENCHMARK: ${BUILD_BENCHMARK}")
# 添加子目录
add_subdirectory(src)
if (BUILD_TESTING)
    add_subdirectory(tests)
endif()
if (BUILD_BENCHMARK)
    add_subdirectory(benchmark)
endif()

# --- 其他 ---------------------------------------------

file(COPY ${CMAKE_SOURCE_DIR}/fronted DESTINATION ${EXECUTABLE_OUTPUT_PATH})

