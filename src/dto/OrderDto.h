#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "model/Order.h"
#include "model/Money.h"

namespace dd::ddmart::dto {

struct OrderCheckoutDto {
    std::string shipping_address;
    std::string payment_method{"MOCK_CARD"};
};

struct OrderStatusUpdateDto {
    std::string status;
};

struct OrderItemResponseDto {
    int64_t id{0};
    int64_t product_id{0};
    std::string product_name;
    std::string product_image_url;
    int32_t quantity{0};
    int64_t unit_price_cents{0};
    std::string unit_price_formatted;
    int64_t subtotal_cents{0};
    std::string subtotal_formatted;

    [[nodiscard]] nlohmann::json ToJson() const {
        return {
            {"id", id},
            {"product_id", product_id},
            {"product_name", product_name},
            {"product_image_url", product_image_url},
            {"quantity", quantity},
            {"unit_price_cents", unit_price_cents},
            {"unit_price_formatted", unit_price_formatted},
            {"subtotal_cents", subtotal_cents},
            {"subtotal_formatted", subtotal_formatted}
        };
    }
};

struct OrderResponseDto {
    int64_t id{0};
    int64_t buyer_id{0};
    std::string buyer_name;
    std::string buyer_email;
    std::string status;
    int64_t total_amount_cents{0};
    std::string total_amount_formatted;
    std::string created_at;
    std::vector<OrderItemResponseDto> items;

    [[nodiscard]] nlohmann::json ToJson() const {
        nlohmann::json items_arr = nlohmann::json::array();
        for (const auto& item : items) {
            items_arr.push_back(item.ToJson());
        }
        return {
            {"id", id},
            {"buyer_id", buyer_id},
            {"buyer_name", buyer_name},
            {"buyer_email", buyer_email},
            {"status", status},
            {"total_amount_cents", total_amount_cents},
            {"total_amount_formatted", total_amount_formatted},
            {"created_at", created_at},
            {"items", items_arr}
        };
    }

    static OrderResponseDto FromEntity(const model::Order& order) {
        OrderResponseDto dto;
        dto.id = order.id;
        dto.buyer_id = order.buyer_id;
        dto.buyer_name = order.buyer_name;
        dto.buyer_email = order.buyer_email;
        dto.status = model::OrderStatusToString(order.status);
        dto.total_amount_cents = order.total_amount.GetCents();
        dto.total_amount_formatted = order.total_amount.ToString();
        dto.created_at = order.created_at;
        for (const auto& item : order.items) {
            OrderItemResponseDto item_dto;
            item_dto.id = item.id;
            item_dto.product_id = item.product_id;
            item_dto.product_name = item.product_name;
            item_dto.product_image_url = item.product_image_url;
            item_dto.quantity = item.quantity;
            item_dto.unit_price_cents = item.unit_price.GetCents();
            item_dto.unit_price_formatted = item.unit_price.ToString();
            item_dto.subtotal_cents = (item.unit_price * item.quantity).GetCents();
            item_dto.subtotal_formatted = (item.unit_price * item.quantity).ToString();
            dto.items.push_back(item_dto);
        }
        return dto;
    }
};

} // namespace dd::ddmart::dto
