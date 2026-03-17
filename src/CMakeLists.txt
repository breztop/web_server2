set(PROJECT_NAME "breWebserverlib")

add_library(${PROJECT_NAME} STATIC)

# 收集各模块源文件
file(GLOB SOURCES 
    "config/*.cpp"
    "http/*.cpp"
    "server/*.cpp"
)

target_sources(${PROJECT_NAME} PRIVATE
    ${SOURCES}
)

if (NEED_POSTGRESQL)
    file(GLOB POSTGRESQL_SOURCES
        "database/*.cpp"
        "service/*.cpp"
    )

    target_sources(${PROJECT_NAME} PRIVATE
        ${POSTGRESQL_SOURCES}
    )

    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_POSTGRESQL=1)

    find_package(OpenSSL REQUIRED)
endif()

include_directories(.)

# 链接库
link_boost_header(${PROJECT_NAME})
link_breutil(${PROJECT_NAME})
if(NEED_POSTGRESQL)
    link_libpqxx(${PROJECT_NAME})
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()

add_executable(breWebserver main.cpp)
target_link_libraries(breWebserver PRIVATE ${PROJECT_NAME})
if (NEED_POSTGRESQL)
    message(STATUS "PostgreSQL support enabled" )
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_POSTGRESQL)
endif()
link_breutil(breWebserver)
generate_build_info(breWebserver)
link_boost_header(breWebserver)