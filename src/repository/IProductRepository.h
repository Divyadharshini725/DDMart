#pragma once

#include <string>
#include <optional>
#include <vector>
#include <memory>
#include "model/Product.h"

namespace dd::ddmart::repository {

struct ProductFilter {
    std::string category;
    std::string keyword;
    std::string sort_by{"created_at_desc"};
    int32_t page{1};
    int32_t limit{20};
};

class IProductRepository {
public:
    virtual ~IProductRepository() = default;

    virtual std::optional<model::Product> FindById(int64_t id) = 0;
    virtual std::vector<model::Product> FindByFilter(const ProductFilter& filter) = 0;
    virtual int64_t CountByFilter(const ProductFilter& filter) = 0;
    virtual std::vector<model::Product> FindBySellerId(int64_t seller_id) = 0;
    virtual model::Product Create(const model::Product& product) = 0;
    virtual bool Update(const model::Product& product) = 0;
    virtual bool Delete(int64_t id) = 0;
    virtual bool DecrementStock(int64_t product_id, int32_t quantity) = 0;
    virtual int64_t Count() = 0;
};

} // namespace dd::ddmart::repository
