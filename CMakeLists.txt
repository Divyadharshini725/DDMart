cmake_minimum_required(VERSION 3.25)
project(DDMart VERSION 1.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0A00)
endif()

# Compiler flags and warnings (Google C++ Style Guide / Clean builds)
if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wno-unused-parameter)
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/third_party
)

# Vendored / System dependency handling (Portable across Windows, Linux, and Docker)
# 1. SQLite3
if(NOT TARGET SQLite::SQLite3)
    find_package(SQLite3 QUIET)
endif()
if(NOT TARGET SQLite::SQLite3)
    add_library(sqlite3_vendored STATIC src/third_party/sqlite3.c)
    set_target_properties(sqlite3_vendored PROPERTIES LINKER_LANGUAGE C)
    target_include_directories(sqlite3_vendored PUBLIC src/third_party)
    add_library(SQLite::SQLite3 ALIAS sqlite3_vendored)
endif()

# 2. nlohmann_json
if(NOT TARGET nlohmann_json::nlohmann_json)
    find_package(nlohmann_json QUIET)
endif()
if(NOT TARGET nlohmann_json::nlohmann_json)
    add_library(nlohmann_json_vendored INTERFACE)
    target_include_directories(nlohmann_json_vendored INTERFACE src/third_party)
    add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json_vendored)
endif()

# 3. spdlog
if(NOT TARGET spdlog::spdlog)
    find_package(spdlog QUIET)
endif()
if(NOT TARGET spdlog::spdlog)
    add_library(spdlog_vendored INTERFACE)
    target_include_directories(spdlog_vendored INTERFACE src/third_party)
    add_library(spdlog::spdlog ALIAS spdlog_vendored)
endif()

# 4. sodium
if(NOT TARGET sodium AND NOT TARGET libsodium)
    add_library(sodium_vendored STATIC src/third_party/sodium.cpp)
    target_include_directories(sodium_vendored PUBLIC src/third_party)
    add_library(sodium ALIAS sodium_vendored)
endif()

# Core executable
set(DDMART_SOURCES
    src/main.cpp
)

add_executable(ddmart ${DDMART_SOURCES})

target_link_libraries(ddmart PRIVATE
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    SQLite::SQLite3
    sodium
)

if(WIN32)
    target_link_libraries(ddmart PRIVATE ws2_32 crypt32 wsock32)
else()
    target_link_libraries(ddmart PRIVATE pthread)
endif()

# Optional MySQL / MariaDB Client Library
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(MYSQL_CLIENT QUIET mariadb)
    if(NOT MYSQL_CLIENT_FOUND)
        pkg_check_modules(MYSQL_CLIENT QUIET mysqlclient)
    endif()
endif()

if(MYSQL_CLIENT_FOUND)
    message(STATUS "Found MySQL/MariaDB client library: ${MYSQL_CLIENT_LIBRARIES}")
    target_include_directories(ddmart PRIVATE ${MYSQL_CLIENT_INCLUDE_DIRS})
    target_link_libraries(ddmart PRIVATE ${MYSQL_CLIENT_LIBRARIES})
    target_compile_definitions(ddmart PRIVATE DDMART_HAS_MYSQL=1)
endif()

# Testing Target (optional, disabled by default for fast & clean cloud container builds)
option(BUILD_TESTING "Build the test suite" OFF)
if(BUILD_TESTING)
    find_package(GTest QUIET)
    if(GTest_FOUND)
        enable_testing()
        add_subdirectory(test)
    else()
        message(STATUS "GTest not found; skipping test targets.")
    endif()
endif()
