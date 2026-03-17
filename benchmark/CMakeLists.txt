

if (UNIX)
    message(STATUS "Configuring benchmark for linux and macOS")
    add_executable(benchmark benchmark.cpp)

    set_target_properties(benchmark PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/run/benchmark"
    )

    link_breutil(benchmark)
    target_include_directories(benchmark PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )
    target_link_libraries(benchmark PRIVATE
        Threads::Threads
    )
    link_benchmark(benchmark)

    # 拷贝 配置文件到输出目录
    add_custom_command(TARGET benchmark POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/config.ini
            ${EXECUTABLE_OUTPUT_PATH}/config.ini
    )
else()
    message(WARNING "Windows to configure benchmark is very troublesome")
endif()

