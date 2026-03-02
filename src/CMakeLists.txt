# 收集各模块源文件
file(GLOB BUFFER_SOURCES "buffer/*.cpp")
file(GLOB CONFIG_SOURCES "config/*.cpp")
file(GLOB DATABASE_SOURCES "database/*.cpp")
file(GLOB HTTP_SOURCES "http/*.cpp")
file(GLOB SERVER_SOURCES "server/*.cpp")
file(GLOB TIMER_SOURCES "timer/*.cpp")
file(GLOB POOL_SOURCES "pool/*.cpp")

# 主程序
add_executable(web_server 
    main.cpp
    ${BUFFER_SOURCES}
    ${CONFIG_SOURCES}
    ${DATABASE_SOURCES}
    ${HTTP_SOURCES}
    ${SERVER_SOURCES}
    ${TIMER_SOURCES}
    ${POOL_SOURCES}
)

# 包含目录
target_include_directories(web_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
)

message("Boost include dirs: ${Boost_INCLUDE_DIRS}")

# 链接库
target_link_libraries(web_server PRIVATE
    Threads::Threads
)

# Windows特定配置
if(WIN32)
    target_link_libraries(web_server PRIVATE
        ws2_32
        wsock32
    )
endif()

# Linux特定配置
if(PLATFORM_LINUX)
    target_include_directories(web_server PRIVATE ${PostgreSQL_INCLUDE_DIRS})
    target_link_libraries(web_server PRIVATE 
        ${PostgreSQL_LIBRARIES}
    )
endif()

# 测试
enable_testing()

# 单元测试列表
set(UNIT_TESTS
    test_buffer
    test_config
    test_http
    test_threadpool
)

# 为每个单元测试创建可执行文件
foreach(TEST_NAME ${UNIT_TESTS})
    add_executable(${TEST_NAME}
        tests/${TEST_NAME}.cpp
        ${BUFFER_SOURCES}
        ${CONFIG_SOURCES}
        ${DATABASE_SOURCES}
        ${HTTP_SOURCES}
        ${TIMER_SOURCES}
        ${POOL_SOURCES}
    )
    
    target_include_directories(${TEST_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${Boost_INCLUDE_DIRS}
    )
    
    target_link_libraries(${TEST_NAME} PRIVATE
        Threads::Threads
        ${Boost_LIBRARIES}
    )
    
    if(WIN32)
        target_link_libraries(${TEST_NAME} PRIVATE ws2_32 wsock32)
    endif()
    
    if(PLATFORM_LINUX)
        target_link_libraries(${TEST_NAME} PRIVATE ${PostgreSQL_LIBRARIES})
    endif()
    
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()

# Benchmark测试（需要服务器运行，不加入自动测试）
if(PLATFORM_LINUX)
    add_executable(benchmark
        tests/benchmark.cpp
    )
    
    target_include_directories(benchmark PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
    
    target_link_libraries(benchmark PRIVATE
        Threads::Threads
    )
endif()


file(COPY ${CMAKE_SOURCE_DIR}/fronted DESTINATION ${EXECUTABLE_OUTPUT_PATH})