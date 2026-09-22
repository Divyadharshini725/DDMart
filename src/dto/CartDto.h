#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "model/CartItem.h"
#include "model/Money.h"

namespace dd::ddmart::dto {

struct CartAddDto {
    int64_t product_id{0};
    int32_t quantity{1};
};

struct CartUpdateDto {
    int32_t quantity{1};
};

struct CartItemResponseDto {
    int64_t id{0};
    int64_t product_id{0};
    std::string product_name;
    int64_t unit_price_cents{0};
    std::string unit_price_formatted;
    int32_t quantity{0};
    int64_t subtotal_cents{0};
    std::string subtotal_formatted;
    int32_t available_stock{0};
    std::string image_url;
    std::string category;

    [[nodiscard]] nlohmann::json ToJson() const {
        return {
            {"id", id},
            {"product_id", product_id},
            {"product_name", product_name},
            {"unit_price_cents", unit_price_cents},
            {"unit_price_formatted", unit_price_formatted},
            {"quantity", quantity},
            {"subtotal_cents", subtotal_cents},
            {"subtotal_formatted", subtotal_formatted},
            {"available_stock", available_stock},
            {"image_url", image_url},
            {"category", category}
        };
    }
};

struct CartResponseDto {
    std::vector<CartItemResponseDto> items;
    int32_t total_items{0};
    int64_t total_amount_cents{0};
    std::string total_amount_formatted;

    [[nodiscard]] nlohmann::json ToJson() const {
        nlohmann::json items_arr = nlohmann::json::array();
        for (const auto& item : items) {
            items_arr.push_back(item.ToJson());
        }
        return {
            {"items", items_arr},
            {"total_items", total_items},
            {"total_amount_cents", total_amount_cents},
            {"total_amount_formatted", total_amount_formatted}
        };
    }
};

} // namespace dd::ddmart::dto
