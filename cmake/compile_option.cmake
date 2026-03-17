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
endif()

# Release下 -O2 优化级别, 尽可能的提升运行速度
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        add_compile_options(/O2)
    else()
        add_compile_options(-O3)
    endif()
endif()

set(FETCHCONTENT_BASE_DIR ${CMAKE_SOURCE_DIR}/run/third_party)