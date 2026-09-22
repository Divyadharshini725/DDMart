#pragma once

#include <stdexcept>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace dd::ddmart::exception {

struct FieldError {
    std::string field;
    std::string message;
};

class ApiException : public std::runtime_error {
public:
    ApiException(int status_code, std::string error_code, std::string message, std::vector<FieldError> field_errors = {})
        : std::runtime_error(message),
          status_code_(status_code),
          error_code_(std::move(error_code)),
          message_(std::move(message)),
          field_errors_(std::move(field_errors)) {}

    [[nodiscard]] int GetStatusCode() const noexcept { return status_code_; }
    [[nodiscard]] const std::string& GetErrorCode() const noexcept { return error_code_; }
    [[nodiscard]] const std::string& GetErrorMessage() const noexcept { return message_; }
    [[nodiscard]] const std::vector<FieldError>& GetFieldErrors() const noexcept { return field_errors_; }

    [[nodiscard]] nlohmann::json ToJson() const {
        nlohmann::json err = {
            {"code", error_code_},
            {"message", message_}
        };
        if (!field_errors_.empty()) {
            nlohmann::json fields = nlohmann::json::array();
            for (const auto& fe : field_errors_) {
                fields.push_back({{"field", fe.field}, {"message", fe.message}});
            }
            err["details"] = fields;
        }
        return {
            {"success", false},
            {"data", nullptr},
            {"error", err}
        };
    }

private:
    int status_code_{500};
    std::string error_code_{"INTERNAL_ERROR"};
    std::string message_;
    std::vector<FieldError> field_errors_;
};

class BadRequestException : public ApiException {
public:
    explicit BadRequestException(std::string message)
        : ApiException(400, "BAD_REQUEST", std::move(message)) {}
};

class ValidationException : public ApiException {
public:
    explicit ValidationException(std::string message, std::vector<FieldError> field_errors = {})
        : ApiException(400, "VALIDATION_ERROR", std::move(message), std::move(field_errors)) {}
};

class UnauthorizedException : public ApiException {
public:
    explicit UnauthorizedException(std::string message = "Authentication required")
        : ApiException(401, "UNAUTHORIZED", std::move(message)) {}
};

class ForbiddenException : public ApiException {
public:
    explicit ForbiddenException(std::string message = "Access forbidden")
        : ApiException(403, "FORBIDDEN", std::move(message)) {}
};

class NotFoundException : public ApiException {
public:
    explicit NotFoundException(std::string message = "Resource not found")
        : ApiException(404, "NOT_FOUND", std::move(message)) {}
};

class ConflictException : public ApiException {
public:
    explicit ConflictException(std::string message)
        : ApiException(409, "CONFLICT", std::move(message)) {}
};

} // namespace dd::ddmart::exception
