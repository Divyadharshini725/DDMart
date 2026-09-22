#pragma once

#include <memory>
#include <string>
#include <vector>
#include "repository/IOrderRepository.h"
#include "repository/ICartRepository.h"
#include "repository/IProductRepository.h"
#include "dto/OrderDto.h"
#include "util/ValidationUtil.h"
#include "exception/ApiException.h"

namespace dd::ddmart::service {

/**
 * @brief Strategy pattern interface for payment processing / mock confirmation.
 */
class IPaymentStrategy {
public:
    virtual ~IPaymentStrategy() = default;
    virtual bool ProcessPayment(int64_t buyer_id, int64_t amount_cents, const std::string& method) = 0;
};

class MockPaymentStrategy : public IPaymentStrategy {
public:
    bool ProcessPayment(int64_t /*buyer_id*/, int64_t /*amount_cents*/, const std::string& /*method*/) override {
        // Mock payment confirmation step per Scope Constraints Section 1
        return true;
    }
};

class OrderService {
public:
    OrderService(
        std::shared_ptr<repository::IOrderRepository> order_repo,
        std::shared_ptr<repository::ICartRepository> cart_repo,
        std::shared_ptr<repository::IProductRepository> product_repo,
        std::shared_ptr<IPaymentStrategy> payment_strategy = std::make_shared<MockPaymentStrategy>())
        : order_repo_(std::move(order_repo)),
          cart_repo_(std::move(cart_repo)),
          product_repo_(std::move(product_repo)),
          payment_strategy_(std::move(payment_strategy)) {}

    dto::OrderResponseDto Checkout(int64_t buyer_id, const dto::OrderCheckoutDto& dto) {
        util::ValidationUtil::ValidateNotEmpty(dto.shipping_address, "shipping_address");

        auto cart_items = cart_repo_->FindByUserId(buyer_id);
        if (cart_items.empty()) {
            throw exception::BadRequestException("Cannot checkout: cart is empty");
        }

        std::vector<model::OrderItem> order_items;
        int64_t total_cents = 0;

        // Verify stock for all items
        for (const auto& ci : cart_items) {
            auto prod = product_repo_->FindById(ci.product_id);
            if (!prod.has_value()) {
                throw exception::NotFoundException("Product with id " + std::to_string(ci.product_id) + " no longer exists");
            }
            if (prod->stock_qty < ci.quantity) {
                throw exception::ValidationException(
                    "Insufficient stock for '" + prod->name + "' (Available: " + std::to_string(prod->stock_qty) + ")",
                    {{"product_id", "Stock unavailable"}}
                );
            }

            model::OrderItem oi;
            oi.product_id = ci.product_id;
            oi.quantity = ci.quantity;
            oi.unit_price = prod->price;
            oi.product_name = prod->name;
            oi.product_image_url = prod->image_url;
            oi.seller_id = prod->seller_id;
            total_cents += (oi.unit_price * oi.quantity).GetCents();

            order_items.push_back(oi);
        }

        // Execute payment strategy (mock payment confirmation step)
        if (!payment_strategy_->ProcessPayment(buyer_id, total_cents, dto.payment_method)) {
            throw exception::BadRequestException("Payment processing failed");
        }

        // Create order and deduct stock atomically
        model::Order created_order = order_repo_->CreateOrderWithItems(buyer_id, order_items);
        return dto::OrderResponseDto::FromEntity(created_order);
    }

    dto::OrderResponseDto GetOrderById(int64_t user_id, const std::string& role, int64_t order_id) {
        auto order = order_repo_->FindById(order_id);
        if (!order.has_value()) {
            throw exception::NotFoundException("Order not found with id " + std::to_string(order_id));
        }

        if (role == "BUYER" && order->buyer_id != user_id) {
            throw exception::ForbiddenException("You do not have permission to view this order");
        }

        if (role == "SELLER") {
            bool has_seller_item = false;
            for (const auto& item : order->items) {
                if (item.seller_id == user_id) {
                    has_seller_item = true;
                    break;
                }
            }
            if (!has_seller_item) {
                throw exception::ForbiddenException("You do not have items in this order");
            }
        }

        return dto::OrderResponseDto::FromEntity(*order);
    }

    std::vector<dto::OrderResponseDto> GetBuyerOrders(int64_t buyer_id) {
        std::vector<dto::OrderResponseDto> dtos;
        auto orders = order_repo_->FindByBuyerId(buyer_id);
        for (const auto& o : orders) {
            dtos.push_back(dto::OrderResponseDto::FromEntity(o));
        }
        return dtos;
    }

    std::vector<dto::OrderResponseDto> GetSellerOrders(int64_t seller_id) {
        std::vector<dto::OrderResponseDto> dtos;
        auto orders = order_repo_->FindBySellerId(seller_id);
        for (const auto& o : orders) {
            dtos.push_back(dto::OrderResponseDto::FromEntity(o));
        }
        return dtos;
    }

    std::vector<dto::OrderResponseDto> GetAllOrders() {
        std::vector<dto::OrderResponseDto> dtos;
        auto orders = order_repo_->FindAll();
        for (const auto& o : orders) {
            dtos.push_back(dto::OrderResponseDto::FromEntity(o));
        }
        return dtos;
    }

    dto::OrderResponseDto UpdateOrderStatus(int64_t order_id, const std::string& new_status_str) {
        auto order = order_repo_->FindById(order_id);
        if (!order.has_value()) {
            throw exception::NotFoundException("Order not found with id " + std::to_string(order_id));
        }

        model::OrderStatus current_status = order->status;
        model::OrderStatus new_status = model::StringToOrderStatus(new_status_str);

        // State machine validation: PENDING -> CONFIRMED -> SHIPPED -> DELIVERED (or CANCELLED)
        if (current_status == model::OrderStatus::kCancelled || current_status == model::OrderStatus::kDelivered) {
            throw exception::BadRequestException("Cannot change status of an order that is already " + model::OrderStatusToString(current_status));
        }

        if (!order_repo_->UpdateStatus(order_id, new_status)) {
            throw std::runtime_error("Failed to update order status");
        }

        auto updated = order_repo_->FindById(order_id);
        return dto::OrderResponseDto::FromEntity(*updated);
    }

    bool HasBuyerPurchasedProduct(int64_t buyer_id, int64_t product_id) {
        return order_repo_->HasBuyerPurchasedProduct(buyer_id, product_id);
    }

    int64_t GetTotalOrderCount() { return order_repo_->Count(); }
    int64_t GetTotalGMVCents() { return order_repo_->TotalGrossMerchandiseValueCents(); }
    int64_t GetSellerRevenueCents(int64_t seller_id) { return order_repo_->SellerRevenueCents(seller_id); }
    int64_t GetSellerOrderCount(int64_t seller_id) { return order_repo_->SellerOrderCount(seller_id); }

private:
    std::shared_ptr<repository::IOrderRepository> order_repo_;
    std::shared_ptr<repository::ICartRepository> cart_repo_;
    std::shared_ptr<repository::IProductRepository> product_repo_;
    std::shared_ptr<IPaymentStrategy> payment_strategy_;
};

} // namespace dd::ddmart::service
