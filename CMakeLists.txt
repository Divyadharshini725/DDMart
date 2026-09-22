cmake_minimum_required(VERSION 3.25)
project(DDMart VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags and warnings (Google C++ Style Guide / Clean builds)
if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

# Find required libraries
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(libsodium CONFIG)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/third_party
)

# Core library source files
set(DDMART_SOURCES
    src/main.cpp
)

add_executable(ddmart ${DDMART_SOURCES})

target_link_libraries(ddmart PRIVATE
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    SQLite::SQLite3
)

if(TARGET libsodium)
    target_link_libraries(ddmart PRIVATE libsodium)
elseif(TARGET sodium)
    target_link_libraries(ddmart PRIVATE sodium)
else()
    target_link_libraries(ddmart PRIVATE -lsodium)
endif()

if(WIN32)
    target_link_libraries(ddmart PRIVATE ws2_32 crypt32 wsock32)
else()
    target_link_libraries(ddmart PRIVATE pthread)
endif()

# Testing Target
enable_testing()
add_subdirectory(test)
