#pragma once

#include <string>
#include <regex>
#include "exception/ApiException.h"

namespace dd::ddmart::util {

class ValidationUtil {
public:
    static bool IsValidEmail(const std::string& email) {
        if (email.empty() || email.length() > 254) return false;
        const std::regex pattern(R"(^[a-zA-Z0-9_.+-]+@[a-zA-Z0-9-]+\.[a-zA-Z0-9-.]+$)");
        return std::regex_match(email, pattern);
    }

    static void ValidateEmail(const std::string& email) {
        if (!IsValidEmail(email)) {
            throw exception::ValidationException("Invalid email format", {{"email", "Must be a valid email address"}});
        }
    }

    static void ValidatePassword(const std::string& password) {
        if (password.length() < 6) {
            throw exception::ValidationException("Password too weak", {{"password", "Password must be at least 6 characters"}});
        }
    }

    static void ValidateNotEmpty(const std::string& val, const std::string& field_name) {
        if (val.empty() || val.find_first_not_of(" \t\n\r") == std::string::npos) {
            throw exception::ValidationException(field_name + " cannot be empty", {{field_name, "This field is required"}});
        }
    }

    static void ValidatePositive(int64_t val, const std::string& field_name) {
        if (val <= 0) {
            throw exception::ValidationException(field_name + " must be positive", {{field_name, "Value must be greater than zero"}});
        }
    }

    static void ValidateNonNegative(int64_t val, const std::string& field_name) {
        if (val < 0) {
            throw exception::ValidationException(field_name + " cannot be negative", {{field_name, "Value must be zero or positive"}});
        }
    }

    static void ValidateRating(int32_t rating) {
        if (rating < 1 || rating > 5) {
            throw exception::ValidationException("Rating must be between 1 and 5", {{"rating", "Rating must be an integer between 1 and 5"}});
        }
    }
};

} // namespace dd::ddmart::util
