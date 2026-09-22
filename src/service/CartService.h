#pragma once

#include <memory>
#include <string>
#include <vector>
#include "repository/ICartRepository.h"
#include "repository/IProductRepository.h"
#include "dto/CartDto.h"
#include "util/ValidationUtil.h"
#include "exception/ApiException.h"

namespace dd::ddmart::service {

class CartService {
public:
    CartService(
        std::shared_ptr<repository::ICartRepository> cart_repo,
        std::shared_ptr<repository::IProductRepository> product_repo)
        : cart_repo_(std::move(cart_repo)),
          product_repo_(std::move(product_repo)) {}

    dto::CartResponseDto GetCart(int64_t user_id) {
        auto items = cart_repo_->FindByUserId(user_id);
        dto::CartResponseDto response;
        int64_t total_cents = 0;
        int32_t total_qty = 0;

        for (const auto& item : items) {
            dto::CartItemResponseDto item_dto;
            item_dto.id = item.id;
            item_dto.product_id = item.product_id;
            item_dto.product_name = item.product_name;
            item_dto.unit_price_cents = item.unit_price.GetCents();
            item_dto.unit_price_formatted = item.unit_price.ToString();
            item_dto.quantity = item.quantity;
            item_dto.available_stock = item.available_stock;
            item_dto.image_url = item.image_url;
            item_dto.category = item.category;

            model::Money subtotal = item.unit_price * item.quantity;
            item_dto.subtotal_cents = subtotal.GetCents();
            item_dto.subtotal_formatted = subtotal.ToString();

            total_cents += subtotal.GetCents();
            total_qty += item.quantity;
            response.items.push_back(item_dto);
        }

        response.total_items = total_qty;
        response.total_amount_cents = total_cents;
        response.total_amount_formatted = model::Money(total_cents).ToString();
        return response;
    }

    void AddToCart(int64_t user_id, const dto::CartAddDto& dto) {
        util::ValidationUtil::ValidatePositive(dto.product_id, "product_id");
        util::ValidationUtil::ValidatePositive(dto.quantity, "quantity");

        auto prod = product_repo_->FindById(dto.product_id);
        if (!prod.has_value()) {
            throw exception::NotFoundException("Product not found with id " + std::to_string(dto.product_id));
        }

        auto existing_item = cart_repo_->FindByUserAndProduct(user_id, dto.product_id);
        int32_t current_qty = existing_item ? existing_item->quantity : 0;
        int32_t desired_qty = current_qty + dto.quantity;

        if (desired_qty > prod->stock_qty) {
            throw exception::ValidationException(
                "Requested quantity exceeds available stock (" + std::to_string(prod->stock_qty) + " available)",
                {{"quantity", "Cannot add more items than currently in stock"}}
            );
        }

        cart_repo_->AddOrUpdate(user_id, dto.product_id, dto.quantity);
    }

    void UpdateQuantity(int64_t user_id, int64_t cart_item_id, const dto::CartUpdateDto& dto) {
        util::ValidationUtil::ValidatePositive(dto.quantity, "quantity");

        auto cart_items = cart_repo_->FindByUserId(user_id);
        const model::CartItem* target = nullptr;
        for (const auto& item : cart_items) {
            if (item.id == cart_item_id) {
                target = &item;
                break;
            }
        }

        if (!target) {
            throw exception::NotFoundException("Cart item not found");
        }

        if (dto.quantity > target->available_stock) {
            throw exception::ValidationException(
                "Requested quantity exceeds available stock (" + std::to_string(target->available_stock) + " available)",
                {{"quantity", "Quantity exceeds available inventory"}}
            );
        }

        cart_repo_->UpdateQuantity(cart_item_id, dto.quantity);
    }

    void RemoveItem(int64_t user_id, int64_t cart_item_id) {
        cart_repo_->Delete(cart_item_id, user_id);
    }

    void ClearCart(int64_t user_id) {
        cart_repo_->ClearByUserId(user_id);
    }

private:
    std::shared_ptr<repository::ICartRepository> cart_repo_;
    std::shared_ptr<repository::IProductRepository> product_repo_;
};

} // namespace dd::ddmart::service
