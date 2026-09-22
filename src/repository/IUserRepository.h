#pragma once

#include <string>
#include <optional>
#include <vector>
#include <memory>
#include "model/User.h"

namespace dd::ddmart::repository {

/**
 * @brief Abstract repository interface for User entity data access.
 */
class IUserRepository {
public:
    virtual ~IUserRepository() = default;

    virtual std::optional<model::User> FindById(int64_t id) = 0;
    virtual std::optional<model::User> FindByEmail(const std::string& email) = 0;
    virtual model::User Create(const model::User& user) = 0;
    virtual std::vector<model::User> FindAll() = 0;
    virtual int64_t Count() = 0;
};

} // namespace dd::ddmart::repository
