#pragma once

#include <string>

namespace dd::ddmart::service {

/**
 * @brief Abstract strategy interface for AI Chat Providers (Specification Section 15).
 * Enables swappable AI backends (Mock, Google Gemini, etc.) through polymorphism.
 */
class ChatProvider {
public:
    virtual ~ChatProvider() = default;

    /**
     * @brief Generates an AI reply for a given user message and domain context.
     * @param user_message The question or prompt supplied by the client.
     * @param context Domain context (catalog info, policies, store metadata).
     * @return Formatted AI reply string.
     */
    virtual std::string GetReply(const std::string& user_message,
                                 const std::string& context) = 0;

    /**
     * @brief Identifies the active provider implementation name.
     * @return Provider identifier string (e.g. "mock", "gemini").
     */
    virtual std::string GetProviderName() const = 0;
};

} // namespace dd::ddmart::service
