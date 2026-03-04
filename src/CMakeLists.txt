set(PROJECT_NAME "breWebserver")


add_executable(${PROJECT_NAME}  main.cpp)

# 收集各模块源文件
file(GLOB SOURCES 
    "config/*.cpp"
    "database/*.cpp"
    "http/*.cpp"
    "server/*.cpp"
)

target_sources(${PROJECT_NAME} PRIVATE
    ${SOURCES}
)

# 链接库
link_boost_header(${PROJECT_NAME})
link_breutil(${PROJECT_NAME})
if(PLATFORM_LINUX)
    link_libpqxx(${PROJECT_NAME})
endif()



# --- 测试 ---------------------------------------------
enable_testing()

# 单元测试列表
set(UNIT_TESTS
    test_buffer
    test_http
    test_threadpool
)

foreach(TEST_NAME ${UNIT_TESTS})
    add_executable(${TEST_NAME}
        tests/${TEST_NAME}.cpp
        ${SOURCES}
    )    
    link_breutil(${TEST_NAME})
    if(PLATFORM_LINUX)
        link_libpqxx(${TEST_NAME})
    endif()
    
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()


# --- Benchmark测试 ---------------------------------------------
add_executable(benchmark
    tests/benchmark.cpp
)

link_breutil(benchmark)
target_include_directories(benchmark PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(benchmark PRIVATE
    Threads::Threads
)
link_benchmark(benchmark)

file(COPY ${CMAKE_SOURCE_DIR}/fronted DESTINATION ${EXECUTABLE_OUTPUT_PATH})