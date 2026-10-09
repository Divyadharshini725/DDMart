#pragma once

#include <memory>
#include <string>
#include <cstdlib>
#include <algorithm>
#include <cctype>
#include "service/ChatProvider.h"
#include "service/MockChatProvider.h"
#include "service/GeminiChatProvider.h"

namespace dd::ddmart::service {

/**
 * @brief Factory class for instantiating AI Chat Providers (Section 10 & Section 15).
 * Selects between 'gemini' and 'mock' based on environment variable or configuration string.
 */
class ChatProviderFactory {
public:
    /**
     * @brief Creates a ChatProvider instance according to the requested provider name or environment.
     * @param requested_provider Optional provider name ("mock", "gemini", or empty for auto-detect).
     * @return Unique pointer to the allocated ChatProvider.
     */
    static std::unique_ptr<ChatProvider> Create(const std::string& requested_provider = "") {
        std::string provider = requested_provider;
        
        if (provider.empty()) {
            const char* env_prov = std::getenv("AI_CHATBOT_PROVIDER");
            if (env_prov != nullptr) {
                provider = env_prov;
            } else {
                provider = "mock";
            }
        }

        std::transform(provider.begin(), provider.end(), provider.begin(), [](unsigned char c) {
            return std::tolower(c);
        });

        if (provider == "gemini") {
            return std::make_unique<GeminiChatProvider>();
        }

        return std::make_unique<MockChatProvider>();
    }
};

} // namespace dd::ddmart::service
