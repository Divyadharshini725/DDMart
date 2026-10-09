#pragma once

#include <string>
#include <cstdlib>
#include <memory>
#include <chrono>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include "service/ChatProvider.h"
#include "service/MockChatProvider.h"
#include "third_party/httplib.h"

namespace dd::ddmart::service {

/**
 * @brief Google Gemini LLM API provider implementation (Section 9 & Section 15).
 * Connects server-side to the Gemini API using httplib with explicit timeouts and guardrails.
 */
class GeminiChatProvider : public ChatProvider {
public:
    GeminiChatProvider() {
        // Requirement 1: API key read server-side only from environment variable at process startup
        const char* key_env = std::getenv("GEMINI_API_KEY");
        if (!key_env) {
            key_env = std::getenv("AI_API_KEY");
        }
        if (key_env != nullptr) {
            api_key_ = std::string(key_env);
        }

        mock_fallback_ = std::make_unique<MockChatProvider>();

        if (api_key_.empty()) {
            spdlog::warn("[GeminiChatProvider] No GEMINI_API_KEY found in environment. Defaulting to Mock fallback.");
        } else {
            spdlog::info("[GeminiChatProvider] Initialized with server-side API key configuration.");
        }
    }

    explicit GeminiChatProvider(std::string api_key)
        : api_key_(std::move(api_key)),
          mock_fallback_(std::make_unique<MockChatProvider>()) {}

    std::string GetProviderName() const override {
        return "gemini";
    }

    std::string GetReply(const std::string& user_message, const std::string& context) override {
        // If API key is missing, seamlessly fall back to domain FAQ
        if (api_key_.empty()) {
            return mock_fallback_->GetReply(user_message, context);
        }

        try {
            // Requirement 2: Fixed server-side prompt template restricting scope to product/listing domain
            std::string prompt = BuildPrompt(user_message, context);

            // Outbound request payload for Gemini generateContent API
            nlohmann::json req_body = {
                {"contents", nlohmann::json::array({
                    {
                        {"role", "user"},
                        {"parts", nlohmann::json::array({
                            {{"text", prompt}}
                        })}
                    }
                })},
                {"generationConfig", {
                    {"temperature", 0.3},
                    {"maxOutputTokens", 256}
                }}
            };

            // Requirement 3: Outbound HTTP call wrapped in explicit timeout (5s) + error handling
            httplib::Client cli("https://generativelanguage.googleapis.com");
            cli.set_connection_timeout(5, 0); // 5 seconds
            cli.set_read_timeout(6, 0);       // 6 seconds

            std::string path = "/v1beta/models/gemini-1.5-flash:generateContent?key=" + api_key_;
            auto res = cli.Post(path.c_str(), req_body.dump(), "application/json");

            if (res && res->status == 200) {
                auto res_json = nlohmann::json::parse(res->body);
                if (res_json.contains("candidates") && 
                    !res_json["candidates"].empty() &&
                    res_json["candidates"][0].contains("content") &&
                    res_json["candidates"][0]["content"].contains("parts") &&
                    !res_json["candidates"][0]["content"]["parts"].empty()) {
                    
                    std::string generated_text = res_json["candidates"][0]["content"]["parts"][0].value("text", "");
                    if (!generated_text.empty()) {
                        return generated_text;
                    }
                }
            } else {
                spdlog::warn("[GeminiChatProvider] Outbound API call failed with status: {}. Using degraded fallback.",
                             res ? std::to_string(res->status) : "connection timeout");
            }
        } catch (const std::exception& e) {
            spdlog::error("[GeminiChatProvider] Exception during outbound call: {}. Using degraded fallback.", e.what());
        }

        // Requirement 3: On failure or timeout, return static degraded response / mock fallback
        return mock_fallback_->GetReply(user_message, context);
    }

private:
    std::string BuildPrompt(const std::string& user_message, const std::string& context) {
        std::ostringstream ss;
        ss << "You are the helpful AI assistant for DDMart, a high-performance native C++ multi-vendor e-commerce marketplace.\n"
           << "GUARDRAIL: Your scope is STRICTLY restricted to product inquiries, listing guidance, order tracking, shipping, and store policies.\n"
           << "If the user asks questions unrelated to shopping or DDMart, politely steer them back to marketplace assistance.\n";
        if (!context.empty()) {
            ss << "\nCurrent Marketplace Context:\n" << context << "\n";
        }
        ss << "\nUser Query: " << user_message << "\nAssistant Answer:";
        return ss.str();
    }

    std::string api_key_;
    std::unique_ptr<MockChatProvider> mock_fallback_;
};

} // namespace dd::ddmart::service
