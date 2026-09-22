#pragma once

#include "repository/ICartRepository.h"
#include "repository/SqliteDatabase.h"

namespace dd::ddmart::repository {

class SqliteCartRepository : public ICartRepository {
public:
    explicit SqliteCartRepository(std::shared_ptr<SqliteDatabase> db)
        : db_(std::move(db)) {}

    std::vector<model::CartItem> FindByUserId(int64_t user_id) override {
        std::lock_guard<std::mutex> lock(db_->GetMutex());
        auto stmt = db_->Prepare(
            "SELECT c.id, c.user_id, c.product_id, c.quantity, "
            "p.name, p.price_cents, p.stock_qty, p.image_url, p.category "
            "FROM cart_items c "
            "JOIN products p ON c.product_id = p.id "
            "WHERE c.user_id = ? "
            "ORDER BY c.id ASC;"
        );
        stmt->BindInt64(1, user_id);

        std::vector<model::CartItem> list;
        while (stmt->Step() == SQLITE_ROW) {
            model::CartItem item;
            item.id = sqlite3_column_int64(stmt->Get(), 0);
            item.user_id = sqlite3_column_int64(stmt->Get(), 1);
            item.product_id = sqlite3_column_int64(stmt->Get(), 2);
            item.quantity = sqlite3_column_int(stmt->Get(), 3);
            item.product_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt->Get(), 4));
            item.unit_price = model::Money(sqlite3_column_int64(stmt->Get(), 5));
            item.available_stock = sqlite3_column_int(stmt->Get(), 6);
            const unsigned char* img = sqlite3_column_text(stmt->Get(), 7);
            item.image_url = img ? reinterpret_cast<const char*>(img) : "";
            item.category = reinterpret_cast<const char*>(sqlite3_column_text(stmt->Get(), 8));
            list.push_back(item);
        }
        return list;
    }

    std::optional<model::CartItem> FindByUserAndProduct(int64_t user_id, int64_t product_id) override {
        std::lock_guard<std::mutex> lock(db_->GetMutex());
        auto stmt = db_->Prepare(
            "SELECT c.id, c.user_id, c.product_id, c.quantity, "
            "p.name, p.price_cents, p.stock_qty, p.image_url, p.category "
            "FROM cart_items c "
            "JOIN products p ON c.product_id = p.id "
            "WHERE c.user_id = ? AND c.product_id = ?;"
        );
        stmt->BindInt64(1, user_id);
        stmt->BindInt64(2, product_id);

        if (stmt->Step() == SQLITE_ROW) {
            model::CartItem item;
            item.id = sqlite3_column_int64(stmt->Get(), 0);
            item.user_id = sqlite3_column_int64(stmt->Get(), 1);
            item.product_id = sqlite3_column_int64(stmt->Get(), 2);
            item.quantity = sqlite3_column_int(stmt->Get(), 3);
            item.product_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt->Get(), 4));
            item.unit_price = model::Money(sqlite3_column_int64(stmt->Get(), 5));
            item.available_stock = sqlite3_column_int(stmt->Get(), 6);
            const unsigned char* img = sqlite3_column_text(stmt->Get(), 7);
            item.image_url = img ? reinterpret_cast<const char*>(img) : "";
            item.category = reinterpret_cast<const char*>(sqlite3_column_text(stmt->Get(), 8));
            return item;
        }
        return std::nullopt;
    }

    void AddOrUpdate(int64_t user_id, int64_t product_id, int32_t quantity) override {
        std::lock_guard<std::mutex> lock(db_->GetMutex());
        auto stmt = db_->Prepare(
            "INSERT INTO cart_items (user_id, product_id, quantity) VALUES (?1, ?2, ?3) "
            "ON CONFLICT(user_id, product_id) DO UPDATE SET quantity = quantity + ?3;"
        );
        stmt->BindInt64(1, user_id);
        stmt->BindInt64(2, product_id);
        stmt->BindInt(3, quantity);
        if (stmt->Step() != SQLITE_DONE) {
            throw std::runtime_error("Failed to add/update item in cart");
        }
    }

    void UpdateQuantity(int64_t cart_item_id, int32_t quantity) override {
        std::lock_guard<std::mutex> lock(db_->GetMutex());
        auto stmt = db_->Prepare("UPDATE cart_items SET quantity = ? WHERE id = ?;");
        stmt->BindInt(1, quantity);
        stmt->BindInt64(2, cart_item_id);
        if (stmt->Step() != SQLITE_DONE) {
            throw std::runtime_error("Failed to update cart quantity");
        }
    }

    void Delete(int64_t cart_item_id, int64_t user_id) override {
        std::lock_guard<std::mutex> lock(db_->GetMutex());
        auto stmt = db_->Prepare("DELETE FROM cart_items WHERE id = ? AND user_id = ?;");
        stmt->BindInt64(1, cart_item_id);
        stmt->BindInt64(2, user_id);
        stmt->Step();
    }

    void ClearByUserId(int64_t user_id) override {
        std::lock_guard<std::mutex> lock(db_->GetMutex());
        auto stmt = db_->Prepare("DELETE FROM cart_items WHERE user_id = ?;");
        stmt->BindInt64(1, user_id);
        stmt->Step();
    }

private:
    std::shared_ptr<SqliteDatabase> db_;
};

} // namespace dd::ddmart::repository
