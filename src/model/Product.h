#pragma once

#include <cstdint>
#include <string>
#include "model/Money.h"

namespace dd::ddmart::model {

struct Product {
    int64_t id{0};
    int64_t seller_id{0};
    std::string name;
    std::string description;
    Money price{0};
    int32_t stock_qty{0};
    std::string category;
    std::string image_url;
    std::string created_at;
    
    // Auxiliary populated fields
    std::string seller_name;
    double average_rating{0.0};
    int32_t review_count{0};
};

} // namespace dd::ddmart::model
