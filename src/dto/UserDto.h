#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include "model/User.h"

namespace dd::ddmart::dto {

struct UserRegisterDto {
    std::string name;
    std::string email;
    std::string password;
    std::string role{"BUYER"};
};

struct UserLoginDto {
    std::string email;
    std::string password;
};

/**
 * @brief Outbound representation of user.
 * MANDATORY: Excludes password_hash under all circumstances.
 */
struct UserResponseDto {
    int64_t id{0};
    std::string name;
    std::string email;
    std::string role;
    std::string created_at;

    [[nodiscard]] nlohmann::json ToJson() const {
        return {
            {"id", id},
            {"name", name},
            {"email", email},
            {"role", role},
            {"created_at", created_at}
        };
    }

    static UserResponseDto FromEntity(const model::User& user) {
        UserResponseDto dto;
        dto.id = user.id;
        dto.name = user.name;
        dto.email = user.email;
        dto.role = model::UserRoleToString(user.role);
        dto.created_at = user.created_at;
        return dto;
    }
};

} // namespace dd::ddmart::dto
