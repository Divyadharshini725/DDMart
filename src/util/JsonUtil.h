#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace dd::ddmart::util {

class JsonUtil {
public:
    static nlohmann::json Success(const nlohmann::json& data) {
        return {
            {"success", true},
            {"data", data},
            {"error", nullptr}
        };
    }

    static nlohmann::json Error(const std::string& code, const std::string& message, const nlohmann::json& details = nullptr) {
        nlohmann::json err = {
            {"code", code},
            {"message", message}
        };
        if (!details.is_null()) {
            err["details"] = details;
        }
        return {
            {"success", false},
            {"data", nullptr},
            {"error", err}
        };
    }
};

} // namespace dd::ddmart::util
