#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace dd::ddmart::dto {

/**
 * @brief Inbound DTO for AI Chatbot request.
 */
struct ChatRequestDto {
    std::string message;
};

/**
 * @brief Outbound DTO for AI Chatbot response.
 */
struct ChatResponseDto {
    std::string reply;
    bool cached{false};
    std::string provider{"mock"};

    [[nodiscard]] nlohmann::json ToJson() const {
        return {
            {"reply", reply},
            {"cached", cached},
            {"provider", provider}
        };
    }
};

} // namespace dd::ddmart::dto
