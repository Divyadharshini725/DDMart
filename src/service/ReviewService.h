#pragma once

#include <memory>
#include <string>
#include <vector>
#include "repository/IReviewRepository.h"
#include "repository/IOrderRepository.h"
#include "repository/IProductRepository.h"
#include "dto/ReviewDto.h"
#include "util/ValidationUtil.h"
#include "exception/ApiException.h"

namespace dd::ddmart::service {

class ReviewService {
public:
    ReviewService(
        std::shared_ptr<repository::IReviewRepository> review_repo,
        std::shared_ptr<repository::IOrderRepository> order_repo,
        std::shared_ptr<repository::IProductRepository> product_repo)
        : review_repo_(std::move(review_repo)),
          order_repo_(std::move(order_repo)),
          product_repo_(std::move(product_repo)) {}

    dto::ReviewResponseDto CreateReview(int64_t user_id, int64_t product_id, const dto::ReviewCreateDto& dto) {
        util::ValidationUtil::ValidateRating(dto.rating);
        util::ValidationUtil::ValidateNotEmpty(dto.comment, "comment");

        auto prod = product_repo_->FindById(product_id);
        if (!prod.has_value()) {
            throw exception::NotFoundException("Product not found with id " + std::to_string(product_id));
        }

        // Must be a verified buyer who completed an order for this product
        if (!order_repo_->HasBuyerPurchasedProduct(user_id, product_id)) {
            throw exception::ForbiddenException("Only verified buyers who completed a purchase of this product may submit a review");
        }

        // Prevent duplicate reviews
        if (review_repo_->FindByProductAndUser(product_id, user_id).has_value()) {
            throw exception::ConflictException("You have already reviewed this product");
        }

        model::Review rev;
        rev.product_id = product_id;
        rev.user_id = user_id;
        rev.rating = dto.rating;
        rev.comment = dto.comment;

        model::Review created = review_repo_->Create(rev);
        return dto::ReviewResponseDto::FromEntity(created);
    }

    std::vector<dto::ReviewResponseDto> GetProductReviews(int64_t product_id) {
        std::vector<dto::ReviewResponseDto> dtos;
        auto reviews = review_repo_->FindByProductId(product_id);
        for (const auto& r : reviews) {
            dtos.push_back(dto::ReviewResponseDto::FromEntity(r));
        }
        return dtos;
    }

private:
    std::shared_ptr<repository::IReviewRepository> review_repo_;
    std::shared_ptr<repository::IOrderRepository> order_repo_;
    std::shared_ptr<repository::IProductRepository> product_repo_;
};

} // namespace dd::ddmart::service
