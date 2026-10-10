#pragma once

#include <iostream>
#include <string>
#include <string_view>
#include <chrono>
#include <format>
#include <mutex>
#include <ctime>

namespace spdlog {

inline std::mutex& LogMutex() {
    static std::mutex mtx;
    return mtx;
}

inline std::string FormatTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
#if defined(_WIN32)
    localtime_s(&tm_buf, &time_t_now);
#else
    localtime_r(&time_t_now, &tm_buf);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_buf);
    return std::string(buf);
}

template <typename... Args>
inline void info(std::format_string<Args...> fmt, Args&&... args) {
    std::lock_guard<std::mutex> lock(LogMutex());
    std::cout << "[" << FormatTimestamp() << "] [info] "
              << std::format(fmt, std::forward<Args>(args)...) << std::endl;
}

template <typename... Args>
inline void warn(std::format_string<Args...> fmt, Args&&... args) {
    std::lock_guard<std::mutex> lock(LogMutex());
    std::cout << "[" << FormatTimestamp() << "] [warning] "
              << std::format(fmt, std::forward<Args>(args)...) << std::endl;
}

template <typename... Args>
inline void error(std::format_string<Args...> fmt, Args&&... args) {
    std::lock_guard<std::mutex> lock(LogMutex());
    std::cerr << "[" << FormatTimestamp() << "] [error] "
              << std::format(fmt, std::forward<Args>(args)...) << std::endl;
}

inline void info(const std::string& msg) {
    std::lock_guard<std::mutex> lock(LogMutex());
    std::cout << "[" << FormatTimestamp() << "] [info] " << msg << std::endl;
}

inline void warn(const std::string& msg) {
    std::lock_guard<std::mutex> lock(LogMutex());
    std::cout << "[" << FormatTimestamp() << "] [warning] " << msg << std::endl;
}

inline void error(const std::string& msg) {
    std::lock_guard<std::mutex> lock(LogMutex());
    std::cerr << "[" << FormatTimestamp() << "] [error] " << msg << std::endl;
}

} // namespace spdlog
