#pragma once

#include <vector>
#include <optional>
#include <memory>
#include "model/CartItem.h"

namespace dd::ddmart::repository {

class ICartRepository {
public:
    virtual ~ICartRepository() = default;

    virtual std::vector<model::CartItem> FindByUserId(int64_t user_id) = 0;
    virtual std::optional<model::CartItem> FindByUserAndProduct(int64_t user_id, int64_t product_id) = 0;
    virtual void AddOrUpdate(int64_t user_id, int64_t product_id, int32_t quantity) = 0;
    virtual void UpdateQuantity(int64_t cart_item_id, int32_t quantity) = 0;
    virtual void Delete(int64_t cart_item_id, int64_t user_id) = 0;
    virtual void ClearByUserId(int64_t user_id) = 0;
};

} // namespace dd::ddmart::repository
