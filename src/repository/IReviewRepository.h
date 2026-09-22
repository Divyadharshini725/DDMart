#pragma once

#include <vector>
#include <optional>
#include <memory>
#include "model/Review.h"

namespace dd::ddmart::repository {

class IReviewRepository {
public:
    virtual ~IReviewRepository() = default;

    virtual std::vector<model::Review> FindByProductId(int64_t product_id) = 0;
    virtual std::optional<model::Review> FindByProductAndUser(int64_t product_id, int64_t user_id) = 0;
    virtual model::Review Create(const model::Review& review) = 0;
    virtual double GetAverageRating(int64_t product_id) = 0;
    virtual int32_t GetReviewCount(int64_t product_id) = 0;
};

} // namespace dd::ddmart::repository
