#pragma once

#include <memory>
#include <string>
#include <vector>
#include "repository/IUserRepository.h"
#include "dto/UserDto.h"
#include "util/ValidationUtil.h"
#include "util/PasswordUtil.h"
#include "exception/ApiException.h"

namespace dd::ddmart::service {

class UserService {
public:
    explicit UserService(std::shared_ptr<repository::IUserRepository> user_repo)
        : user_repo_(std::move(user_repo)) {}

    dto::UserResponseDto Register(const dto::UserRegisterDto& dto) {
        // Validation occurs at top of method before any repository call
        util::ValidationUtil::ValidateNotEmpty(dto.name, "name");
        util::ValidationUtil::ValidateEmail(dto.email);
        util::ValidationUtil::ValidatePassword(dto.password);

        std::string role_str = dto.role;
        if (role_str != "BUYER" && role_str != "SELLER") {
            throw exception::ValidationException("Invalid role selected", {{"role", "Role must be BUYER or SELLER"}});
        }

        if (user_repo_->FindByEmail(dto.email).has_value()) {
            throw exception::ConflictException("Email is already registered: " + dto.email);
        }

        model::User user;
        user.name = dto.name;
        user.email = dto.email;
        user.password_hash = util::PasswordUtil::HashPassword(dto.password);
        user.role = model::StringToUserRole(role_str);

        model::User created = user_repo_->Create(user);
        return dto::UserResponseDto::FromEntity(created);
    }

    dto::UserResponseDto Login(const dto::UserLoginDto& dto) {
        util::ValidationUtil::ValidateEmail(dto.email);
        util::ValidationUtil::ValidateNotEmpty(dto.password, "password");

        auto user_opt = user_repo_->FindByEmail(dto.email);
        if (!user_opt.has_value()) {
            throw exception::UnauthorizedException("Invalid email or password");
        }

        const auto& user = *user_opt;
        if (!util::PasswordUtil::VerifyPassword(dto.password, user.password_hash)) {
            throw exception::UnauthorizedException("Invalid email or password");
        }

        return dto::UserResponseDto::FromEntity(user);
    }

    dto::UserResponseDto GetUserById(int64_t id) {
        auto user_opt = user_repo_->FindById(id);
        if (!user_opt.has_value()) {
            throw exception::NotFoundException("User not found with id " + std::to_string(id));
        }
        return dto::UserResponseDto::FromEntity(*user_opt);
    }

    std::vector<dto::UserResponseDto> GetAllUsers() {
        std::vector<dto::UserResponseDto> list;
        for (const auto& u : user_repo_->FindAll()) {
            list.push_back(dto::UserResponseDto::FromEntity(u));
        }
        return list;
    }

    int64_t GetTotalUserCount() {
        return user_repo_->Count();
    }

private:
    std::shared_ptr<repository::IUserRepository> user_repo_;
};

} // namespace dd::ddmart::service
