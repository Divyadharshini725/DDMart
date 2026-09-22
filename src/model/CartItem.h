#pragma once

#include <cstdint>
#include <string>
#include "model/Money.h"

namespace dd::ddmart::model {

struct CartItem {
    int64_t id{0};
    int64_t user_id{0};
    int64_t product_id{0};
    int32_t quantity{0};

    // Auxiliary joined product fields for display
    std::string product_name;
    Money unit_price{0};
    int32_t available_stock{0};
    std::string image_url;
    std::string category;
};

} // namespace dd::ddmart::model
