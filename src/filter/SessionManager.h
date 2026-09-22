#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>
#include <optional>

namespace dd::ddmart::filter {

struct SessionData {
    int64_t user_id{0};
    std::string email;
    std::string name;
    std::string role; // BUYER, SELLER, ADMIN
    std::chrono::steady_clock::time_point last_accessed;
};

class SessionManager {
public:
    static SessionManager& Instance() {
        static SessionManager instance;
        return instance;
    }

    std::string CreateSession(int64_t user_id, const std::string& email, const std::string& name, const std::string& role) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string session_id = GenerateId();
        SessionData data;
        data.user_id = user_id;
        data.email = email;
        data.name = name;
        data.role = role;
        data.last_accessed = std::chrono::steady_clock::now();
        sessions_[session_id] = data;
        return session_id;
    }

    std::optional<SessionData> GetSession(const std::string& session_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = sessions_.find(session_id);
        if (it == sessions_.end()) {
            return std::nullopt;
        }

        // Check idle timeout (60 minutes)
        auto now = std::chrono::steady_clock::now();
        auto idle_time = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.last_accessed).count();
        if (idle_time > 3600) {
            sessions_.erase(it);
            return std::nullopt;
        }

        it->second.last_accessed = now;
        return it->second;
    }

    void InvalidateSession(const std::string& session_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        sessions_.erase(session_id);
    }

private:
    SessionManager() = default;

    std::string GenerateId() {
        std::random_device rd;
        std::mt19937_64 gen(rd());
        std::uniform_int_distribution<uint64_t> dis;
        std::ostringstream oss;
        oss << std::hex << std::setfill('0') << std::setw(16) << dis(gen)
            << std::setw(16) << dis(gen);
        return oss.str();
    }

    std::unordered_map<std::string, SessionData> sessions_;
    std::mutex mutex_;
};

} // namespace dd::ddmart::filter
