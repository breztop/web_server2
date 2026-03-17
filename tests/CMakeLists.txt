
set(UNIT_TESTS
    test_buffer
    test_http
    test_threadpool
    test_session
    test_http_methods
    test_http_status
    test_content_type
)

include_directories(${CMAKE_SOURCE_DIR}/src)

if (NEED_POSTGRESQL)
    find_package(OpenSSL REQUIRED)
endif()

foreach(TEST_NAME ${UNIT_TESTS})
    add_executable(${TEST_NAME}
        ${TEST_NAME}.cpp
        ${SOURCES}
    )
    target_link_libraries(${TEST_NAME} PRIVATE breWebserverlib)
    set_target_properties(${TEST_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/run/tests"
    )
    link_breutil(${TEST_NAME})
    if(NEED_POSTGRESQL)
        link_libpqxx(${TEST_NAME})
        target_link_libraries(${TEST_NAME} PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    endif()
    
    add_test(NAME ${TEST_NAME} COMMAND $<TARGET_FILE:${TEST_NAME}>)
    set_tests_properties(${TEST_NAME} PROPERTIES
        TIMEOUT 10
    )
endforeach()