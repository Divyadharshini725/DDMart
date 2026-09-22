#pragma once

#include <cstdint>
#include <string>

namespace dd::ddmart::model {

enum class UserRole {
    kBuyer,
    kSeller,
    kAdmin
};

inline std::string UserRoleToString(UserRole role) {
    switch (role) {
        case UserRole::kBuyer: return "BUYER";
        case UserRole::kSeller: return "SELLER";
        case UserRole::kAdmin: return "ADMIN";
    }
    return "BUYER";
}

inline UserRole StringToUserRole(const std::string& str) {
    if (str == "SELLER") return UserRole::kSeller;
    if (str == "ADMIN") return UserRole::kAdmin;
    return UserRole::kBuyer;
}

struct User {
    int64_t id{0};
    std::string name;
    std::string email;
    std::string password_hash;
    UserRole role{UserRole::kBuyer};
    std::string created_at;
};

} // namespace dd::ddmart::model
