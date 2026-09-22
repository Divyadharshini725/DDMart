#pragma once

#include <cstdint>
#include <string>

namespace dd::ddmart::model {

struct Review {
    int64_t id{0};
    int64_t product_id{0};
    int64_t user_id{0};
    int32_t rating{5};
    std::string comment;
    std::string created_at;

    // Joined reviewer display name
    std::string user_name;
    std::string product_name;
};

} // namespace dd::ddmart::model
