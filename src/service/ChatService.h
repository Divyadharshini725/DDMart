#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <chrono>
#include <sstream>
#include <algorithm>
#include <spdlog/spdlog.h>

#include "service/ChatProvider.h"
#include "repository/IProductRepository.h"
#include "dto/ChatDto.h"
#include "exception/ApiException.h"

namespace dd::ddmart::service {

/**
 * @brief Rate limit state entry per session.
 */
struct SessionRateLimit {
    int32_t count{0};
    std::chrono::steady_clock::time_point window_start;
};

/**
 * @brief Orchestrates AI Chatbot business logic, guardrails, rate limiting, and caching.
 * Fulfills Phase 3 AI Feature Specification (Sections 9, 10, 15).
 */
class ChatService {
public:
    static constexpr size_t kMaxMessageLength = 500;
    static constexpr int32_t kMaxMessagesPerMinute = 10;

    /**
     * @brief Constructs ChatService with swappable AI provider and product repository.
     * @param provider Shared pointer to the polymorphic ChatProvider.
     * @param product_repo Optional shared pointer to product repository for catalog context.
     */
    explicit ChatService(
        std::shared_ptr<ChatProvider> provider,
        std::shared_ptr<repository::IProductRepository> product_repo = nullptr)
        : provider_(std::move(provider)),
          product_repo_(std::move(product_repo)) {}

    /**
     * @brief Processes an incoming chat message with rate limiting, validation, and caching.
     * @param session_id The unique session identifier for the requester.
     * @param raw_message The raw text message input.
     * @return ChatResponseDto containing reply, cache status, and provider name.
     */
    dto::ChatResponseDto ProcessMessage(const std::string& session_id, const std::string& raw_message) {
        // 1. Guardrail: Validate and sanitize input length
        std::string trimmed = Trim(raw_message);
        if (trimmed.empty()) {
            throw exception::ValidationException("Message cannot be empty or solely whitespace");
        }
        if (trimmed.length() > kMaxMessageLength) {
            throw exception::ValidationException(
                "Message exceeds maximum allowed length of " + std::to_string(kMaxMessageLength) + " characters");
        }

        // 2. Guardrail: Enforce per-session rate limit (10 messages/minute)
        EnforceRateLimit(session_id);

        // 3. Cache Check: Check if this session already asked the identical question
        std::string normalized_q = ToLower(trimmed);
        {
            std::lock_guard<std::mutex> lock(cache_mutex_);
            auto it_sess = session_cache_.find(session_id);
            if (it_sess != session_cache_.end()) {
                auto it_ans = it_sess->second.find(normalized_q);
                if (it_ans != it_sess->second.end()) {
                    dto::ChatResponseDto cached_dto;
                    cached_dto.reply = it_ans->second;
                    cached_dto.cached = true;
                    cached_dto.provider = provider_ ? provider_->GetProviderName() : "cached";
                    return cached_dto;
                }
            }
        }

        // 4. Build catalog context if product repository is present
        std::string context = BuildDomainContext();

        // 5. Generate AI reply via configured provider with degraded fallback
        std::string reply;
        try {
            if (provider_) {
                reply = provider_->GetReply(trimmed, context);
            } else {
                reply = GetDegradedFallbackReply();
            }
        } catch (const std::exception& e) {
            spdlog::error("[ChatService] Exception in ChatProvider: {}. Returning degraded fallback.", e.what());
            reply = GetDegradedFallbackReply();
        }

        if (reply.empty()) {
            reply = GetDegradedFallbackReply();
        }

        // 6. Store in session cache
        {
            std::lock_guard<std::mutex> lock(cache_mutex_);
            session_cache_[session_id][normalized_q] = reply;
        }

        dto::ChatResponseDto resp;
        resp.reply = reply;
        resp.cached = false;
        resp.provider = provider_ ? provider_->GetProviderName() : "fallback";
        return resp;
    }

    /**
     * @brief Checks whether the session has exceeded the 10 messages/minute rate limit.
     * @param session_id The unique session identifier.
     * @return True if allowed, false if limit exceeded.
     */
    bool CheckRateLimit(const std::string& session_id) {
        std::lock_guard<std::mutex> lock(rate_limit_mutex_);
        auto now = std::chrono::steady_clock::now();
        auto it = rate_limit_map_.find(session_id);

        if (it == rate_limit_map_.end()) {
            return true;
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.window_start).count();
        if (elapsed >= 60) {
            return true;
        }

        return it->second.count < kMaxMessagesPerMinute;
    }

    /**
     * @brief Resets rate limits and cache for a given session (useful for test suites).
     */
    void ResetSession(const std::string& session_id) {
        {
            std::lock_guard<std::mutex> lock(rate_limit_mutex_);
            rate_limit_map_.erase(session_id);
        }
        {
            std::lock_guard<std::mutex> lock(cache_mutex_);
            session_cache_.erase(session_id);
        }
    }

private:
    void EnforceRateLimit(const std::string& session_id) {
        std::lock_guard<std::mutex> lock(rate_limit_mutex_);
        auto now = std::chrono::steady_clock::now();
        auto& entry = rate_limit_map_[session_id];

        if (entry.count == 0) {
            entry.window_start = now;
            entry.count = 1;
            return;
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - entry.window_start).count();
        if (elapsed >= 60) {
            // New 1-minute window
            entry.window_start = now;
            entry.count = 1;
        } else {
            if (entry.count >= kMaxMessagesPerMinute) {
                throw exception::ApiException(
                    429,
                    "RATE_LIMIT_EXCEEDED",
                    "Too many chat requests. Rate limit is " + std::to_string(kMaxMessagesPerMinute) + " messages per minute."
                );
            }
            entry.count++;
        }
    }

    std::string BuildDomainContext() {
        std::ostringstream ss;
        ss << "DDMart is a multi-vendor marketplace featuring categories: Electronics, Fashion, Home & Living.\n"
           << "Return Policy: 30-day money-back guarantee.\n"
           << "Order Workflow: PENDING -> CONFIRMED -> SHIPPED -> DELIVERED.\n"
           << "Security: Passwords secured with Argon2id via libsodium.\n";

        if (product_repo_) {
            try {
                repository::ProductFilter filter;
                filter.limit = 5;
                auto prods = product_repo_->FindByFilter(filter);
                if (!prods.empty()) {
                    ss << "Popular featured items:\n";
                    for (const auto& p : prods) {
                        ss << "- " << p.name << " (" << p.category << "): " << p.price.ToString() << "\n";
                    }
                }
            } catch (...) {
                // Ignore context build exceptions
            }
        }
        return ss.str();
    }

    static std::string GetDegradedFallbackReply() {
        return "I am currently running in offline assistance mode. "
               "For questions about orders, please visit 'My Orders'. "
               "For inquiries on returns, DDMart offers a 30-day return policy. "
               "You can reach customer support at support@ddmart.com.";
    }

    static std::string Trim(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(" \t\r\n");
        return str.substr(first, (last - first + 1));
    }

    static std::string ToLower(const std::string& str) {
        std::string res = str;
        std::transform(res.begin(), res.end(), res.begin(), [](unsigned char c) {
            return std::tolower(c);
        });
        return res;
    }

    std::shared_ptr<ChatProvider> provider_;
    std::shared_ptr<repository::IProductRepository> product_repo_;

    std::unordered_map<std::string, SessionRateLimit> rate_limit_map_;
    std::mutex rate_limit_mutex_;

    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> session_cache_;
    std::mutex cache_mutex_;
};

} // namespace dd::ddmart::service
