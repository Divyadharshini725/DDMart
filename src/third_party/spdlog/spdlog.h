#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
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

// Portable placeholder replacement for "{}" without depending on GCC 13 <format>
inline std::string FormatString(const std::string& fmt) {
    return fmt;
}

template <typename T, typename... Args>
inline std::string FormatString(const std::string& fmt, T&& val, Args&&... rest) {
    size_t pos = fmt.find("{}");
    if (pos == std::string::npos) {
        return fmt;
    }
    std::ostringstream ss;
    ss << std::forward<T>(val);
    std::string prefix = fmt.substr(0, pos);
    std::string suffix = fmt.substr(pos + 2);
    return prefix + ss.str() + FormatString(suffix, std::forward<Args>(rest)...);
}

template <typename... Args>
inline void info(const std::string& fmt, Args&&... args) {
    std::lock_guard<std::mutex> lock(LogMutex());
    std::cout << "[" << FormatTimestamp() << "] [info] "
              << FormatString(fmt, std::forward<Args>(args)...) << std::endl;
}

template <typename... Args>
inline void warn(const std::string& fmt, Args&&... args) {
    std::lock_guard<std::mutex> lock(LogMutex());
    std::cout << "[" << FormatTimestamp() << "] [warning] "
              << FormatString(fmt, std::forward<Args>(args)...) << std::endl;
}

template <typename... Args>
inline void error(const std::string& fmt, Args&&... args) {
    std::lock_guard<std::mutex> lock(LogMutex());
    std::cerr << "[" << FormatTimestamp() << "] [error] "
              << FormatString(fmt, std::forward<Args>(args)...) << std::endl;
}

inline void info(const char* msg) {
    info(std::string(msg));
}

inline void warn(const char* msg) {
    warn(std::string(msg));
}

inline void error(const char* msg) {
    error(std::string(msg));
}

} // namespace spdlog
