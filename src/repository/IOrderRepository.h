#pragma once

#include <vector>
#include <optional>
#include <memory>
#include "model/Order.h"

namespace dd::ddmart::repository {

class IOrderRepository {
public:
    virtual ~IOrderRepository() = default;

    virtual std::optional<model::Order> FindById(int64_t id) = 0;
    virtual std::vector<model::Order> FindByBuyerId(int64_t buyer_id) = 0;
    virtual std::vector<model::Order> FindBySellerId(int64_t seller_id) = 0;
    virtual std::vector<model::Order> FindAll() = 0;
    virtual model::Order CreateOrderWithItems(int64_t buyer_id, const std::vector<model::OrderItem>& items) = 0;
    virtual bool UpdateStatus(int64_t order_id, model::OrderStatus status) = 0;
    virtual bool HasBuyerPurchasedProduct(int64_t buyer_id, int64_t product_id) = 0;
    virtual int64_t Count() = 0;
    virtual int64_t TotalGrossMerchandiseValueCents() = 0;
    virtual int64_t SellerRevenueCents(int64_t seller_id) = 0;
    virtual int64_t SellerOrderCount(int64_t seller_id) = 0;
};

} // namespace dd::ddmart::repository
