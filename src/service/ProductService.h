#pragma once

#include <memory>
#include <string>
#include <vector>
#include "repository/IProductRepository.h"
#include "dto/ProductDto.h"
#include "util/ValidationUtil.h"
#include "exception/ApiException.h"

namespace dd::ddmart::service {

class ProductService {
public:
    explicit ProductService(std::shared_ptr<repository::IProductRepository> product_repo)
        : product_repo_(std::move(product_repo)) {}

    dto::ProductResponseDto GetProductById(int64_t id) {
        auto prod = product_repo_->FindById(id);
        if (!prod.has_value()) {
            throw exception::NotFoundException("Product not found with id " + std::to_string(id));
        }
        return dto::ProductResponseDto::FromEntity(*prod);
    }

    std::vector<dto::ProductResponseDto> SearchProducts(const repository::ProductFilter& filter) {
        std::vector<dto::ProductResponseDto> dtos;
        auto products = product_repo_->FindByFilter(filter);
        for (const auto& p : products) {
            dtos.push_back(dto::ProductResponseDto::FromEntity(p));
        }
        return dtos;
    }

    int64_t CountProducts(const repository::ProductFilter& filter) {
        return product_repo_->CountByFilter(filter);
    }

    std::vector<dto::ProductResponseDto> GetSellerProducts(int64_t seller_id) {
        std::vector<dto::ProductResponseDto> dtos;
        auto products = product_repo_->FindBySellerId(seller_id);
        for (const auto& p : products) {
            dtos.push_back(dto::ProductResponseDto::FromEntity(p));
        }
        return dtos;
    }

    dto::ProductResponseDto CreateProduct(int64_t seller_id, const dto::ProductCreateDto& dto) {
        util::ValidationUtil::ValidateNotEmpty(dto.name, "name");
        util::ValidationUtil::ValidateNotEmpty(dto.category, "category");
        util::ValidationUtil::ValidatePositive(dto.price_cents, "price_cents");
        util::ValidationUtil::ValidateNonNegative(dto.stock_qty, "stock_qty");

        model::Product prod;
        prod.seller_id = seller_id;
        prod.name = dto.name;
        prod.description = dto.description;
        prod.price = model::Money(dto.price_cents);
        prod.stock_qty = dto.stock_qty;
        prod.category = dto.category;
        prod.image_url = dto.image_url.empty() ? "https://images.unsplash.com/photo-1523275335684-37898b6baf30?w=500&q=80" : dto.image_url;

        model::Product created = product_repo_->Create(prod);
        return dto::ProductResponseDto::FromEntity(created);
    }

    dto::ProductResponseDto UpdateProduct(int64_t seller_id, int64_t product_id, const dto::ProductUpdateDto& dto) {
        auto existing = product_repo_->FindById(product_id);
        if (!existing.has_value()) {
            throw exception::NotFoundException("Product not found with id " + std::to_string(product_id));
        }

        if (existing->seller_id != seller_id) {
            throw exception::ForbiddenException("You do not have permission to modify this product listing");
        }

        util::ValidationUtil::ValidateNotEmpty(dto.name, "name");
        util::ValidationUtil::ValidateNotEmpty(dto.category, "category");
        util::ValidationUtil::ValidatePositive(dto.price_cents, "price_cents");
        util::ValidationUtil::ValidateNonNegative(dto.stock_qty, "stock_qty");

        model::Product prod = *existing;
        prod.name = dto.name;
        prod.description = dto.description;
        prod.price = model::Money(dto.price_cents);
        prod.stock_qty = dto.stock_qty;
        prod.category = dto.category;
        if (!dto.image_url.empty()) {
            prod.image_url = dto.image_url;
        }

        if (!product_repo_->Update(prod)) {
            throw std::runtime_error("Failed to update product");
        }

        auto updated = product_repo_->FindById(product_id);
        return dto::ProductResponseDto::FromEntity(updated.value_or(prod));
    }

    void DeleteProduct(int64_t user_id, const std::string& role, int64_t product_id) {
        auto existing = product_repo_->FindById(product_id);
        if (!existing.has_value()) {
            throw exception::NotFoundException("Product not found with id " + std::to_string(product_id));
        }

        // Sellers can only delete their own products; Admin can delete any (moderation)
        if (role != "ADMIN" && existing->seller_id != user_id) {
            throw exception::ForbiddenException("You do not have permission to delete this product listing");
        }

        if (!product_repo_->Delete(product_id)) {
            throw std::runtime_error("Failed to delete product");
        }
    }

    int64_t GetTotalProductCount() {
        return product_repo_->Count();
    }

private:
    std::shared_ptr<repository::IProductRepository> product_repo_;
};

} // namespace dd::ddmart::service
