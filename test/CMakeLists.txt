find_package(GTest REQUIRED)

set(TEST_SOURCES
    test_main.cpp
    unit/test_money.cpp
    unit/test_validation.cpp
    repository/test_sqlite_repositories.cpp
)

add_executable(ddmart_tests ${TEST_SOURCES})

target_include_directories(ddmart_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/third_party
)

target_link_libraries(ddmart_tests PRIVATE
    GTest::gtest
    GTest::gmock
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    SQLite::SQLite3
)

if(TARGET libsodium)
    target_link_libraries(ddmart_tests PRIVATE libsodium)
elseif(TARGET sodium)
    target_link_libraries(ddmart_tests PRIVATE sodium)
else()
    target_link_libraries(ddmart_tests PRIVATE -lsodium)
endif()

if(WIN32)
    target_link_libraries(ddmart_tests PRIVATE ws2_32 crypt32 wsock32)
else()
    target_link_libraries(ddmart_tests PRIVATE pthread)
endif()

add_test(NAME ddmart_all_tests COMMAND ddmart_tests)
