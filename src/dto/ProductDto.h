#pragma once

#include <string>
#include <nlohmann/json.hpp>
#include "model/Product.h"
#include "model/Money.h"

namespace dd::ddmart::dto {

struct ProductCreateDto {
    std::string name;
    std::string description;
    int64_t price_cents{0};
    int32_t stock_qty{0};
    std::string category;
    std::string image_url;
};

struct ProductUpdateDto {
    std::string name;
    std::string description;
    int64_t price_cents{0};
    int32_t stock_qty{0};
    std::string category;
    std::string image_url;
};

struct ProductResponseDto {
    int64_t id{0};
    int64_t seller_id{0};
    std::string seller_name;
    std::string name;
    std::string description;
    int64_t price_cents{0};
    std::string price_formatted;
    int32_t stock_qty{0};
    std::string category;
    std::string image_url;
    std::string created_at;
    double average_rating{0.0};
    int32_t review_count{0};

    [[nodiscard]] nlohmann::json ToJson() const {
        return {
            {"id", id},
            {"seller_id", seller_id},
            {"seller_name", seller_name},
            {"name", name},
            {"description", description},
            {"price_cents", price_cents},
            {"price_formatted", price_formatted},
            {"stock_qty", stock_qty},
            {"category", category},
            {"image_url", image_url},
            {"created_at", created_at},
            {"average_rating", average_rating},
            {"review_count", review_count}
        };
    }

    static ProductResponseDto FromEntity(const model::Product& product) {
        ProductResponseDto dto;
        dto.id = product.id;
        dto.seller_id = product.seller_id;
        dto.seller_name = product.seller_name;
        dto.name = product.name;
        dto.description = product.description;
        dto.price_cents = product.price.GetCents();
        dto.price_formatted = product.price.ToString();
        dto.stock_qty = product.stock_qty;
        dto.category = product.category;
        dto.image_url = product.image_url;
        dto.created_at = product.created_at;
        dto.average_rating = product.average_rating;
        dto.review_count = product.review_count;
        return dto;
    }
};

} // namespace dd::ddmart::dto
