#pragma once

#include "repository/IOrderRepository.h"
#include "repository/SqliteDatabase.h"

namespace dd::ddmart::repository {

class SqliteOrderRepository : public IOrderRepository {
public:
    explicit SqliteOrderRepository(std::shared_ptr<SqliteDatabase> db)
        : db_(std::move(db)) {}

    std::optional<model::Order> FindById(int64_t id) override {
        std::lock_guard<std::mutex> lock(db_->GetMutex());
        auto stmt = db_->Prepare(
            "SELECT o.id, o.buyer_id, o.status, o.total_amount_cents, o.created_at, "
            "u.name as buyer_name, u.email as buyer_email "
            "FROM orders o "
            "JOIN users u ON o.buyer_id = u.id "
            "WHERE o.id = ?;"
        );
        stmt->BindInt64(1, id);

        if (stmt->Step() == SQLITE_ROW) {
            model::Order order = RowToOrder(stmt->Get());
            PopulateOrderItems(order);
            return order;
        }
        return std::nullopt;
    }

    std::vector<model::Order> FindByBuyerId(int64_t buyer_id) override {
        std::lock_guard<std::mutex> lock(db_->GetMutex());
        auto stmt = db_->Prepare(
            "SELECT o.id, o.buyer_id, o.status, o.total_amount_cents, o.created_at, "
            "u.name as buyer_name, u.email as buyer_email "
            "FROM orders o "
            "JOIN users u ON o.buyer_id = u.id "
            "WHERE o.buyer_id = ? "
            "ORDER BY o.id DESC;"
        );
        stmt->BindInt64(1, buyer_id);

        std::vector<model::Order> list;
        while (stmt->Step() == SQLITE_ROW) {
            list.push_back(RowToOrder(stmt->Get()));
        }
        for (auto& o : list) {
            PopulateOrderItems(o);
        }
        return list;
    }

    std::vector<model::Order> FindBySellerId(int64_t seller_id) override {
        std::lock_guard<std::mutex> lock(db_->GetMutex());
        auto stmt = db_->Prepare(
            "SELECT DISTINCT o.id, o.buyer_id, o.status, o.total_amount_cents, o.created_at, "
            "u.name as buyer_name, u.email as buyer_email "
            "FROM orders o "
            "JOIN users u ON o.buyer_id = u.id "
            "JOIN order_items oi ON o.id = oi.order_id "
            "JOIN products p ON oi.product_id = p.id "
            "WHERE p.seller_id = ? "
            "ORDER BY o.id DESC;"
        );
        stmt->BindInt64(1, seller_id);

        std::vector<model::Order> list;
        while (stmt->Step() == SQLITE_ROW) {
            list.push_back(RowToOrder(stmt->Get()));
        }
        for (auto& o : list) {
            PopulateOrderItems(o);
        }
        return list;
    }

    std::vector<model::Order> FindAll() override {
        std::lock_guard<std::mutex> lock(db_->GetMutex());
        auto stmt = db_->Prepare(
            "SELECT o.id, o.buyer_id, o.status, o.total_amount_cents, o.created_at, "
            "u.name as buyer_name, u.email as buyer_email "
            "FROM orders o "
            "JOIN users u ON o.buyer_id = u.id "
            "ORDER BY o.id DESC;"
        );

        std::vector<model::Order> list;
        while (stmt->Step() == SQLITE_ROW) {
            list.push_back(RowToOrder(stmt->Get()));
        }
        for (auto& o : list) {
            PopulateOrderItems(o);
        }
        return list;
    }

    model::Order CreateOrderWithItems(int64_t buyer_id, const std::vector<model::OrderItem>& items) override {
        std::lock_guard<std::mutex> lock(db_->GetMutex());
        auto tx = db_->BeginTransaction();

        int64_t total_cents = 0;
        for (const auto& item : items) {
            total_cents += (item.unit_price * item.quantity).GetCents();
        }

        // Insert Order
        auto order_stmt = db_->Prepare(
            "INSERT INTO orders (buyer_id, status, total_amount_cents) VALUES (?, 'CONFIRMED', ?);"
        );
        order_stmt->BindInt64(1, buyer_id);
        order_stmt->BindInt64(2, total_cents);

        if (order_stmt->Step() != SQLITE_DONE) {
            throw std::runtime_error("Failed to insert order");
        }
        int64_t order_id = db_->GetLastInsertRowId();

        // Insert Order Items and decrement stock
        for (const auto& item : items) {
            auto item_stmt = db_->Prepare(
                "INSERT INTO order_items (order_id, product_id, quantity, unit_price_cents) VALUES (?, ?, ?, ?);"
            );
            item_stmt->BindInt64(1, order_id);
            item_stmt->BindInt64(2, item.product_id);
            item_stmt->BindInt(3, item.quantity);
            item_stmt->BindInt64(4, item.unit_price.GetCents());

            if (item_stmt->Step() != SQLITE_DONE) {
                throw std::runtime_error("Failed to insert order item");
            }

            // Decrement stock
            auto stock_stmt = db_->Prepare(
                "UPDATE products SET stock_qty = stock_qty - ? WHERE id = ? AND stock_qty >= ?;"
            );
            stock_stmt->BindInt(1, item.quantity);
            stock_stmt->BindInt64(2, item.product_id);
            stock_stmt->BindInt(3, item.quantity);

            if (stock_stmt->Step() != SQLITE_DONE || sqlite3_changes(db_->GetRawHandle()) == 0) {
                throw std::runtime_error("Insufficient stock for product id " + std::to_string(item.product_id));
            }
        }

        // Clear buyer's cart
        auto clear_stmt = db_->Prepare("DELETE FROM cart_items WHERE user_id = ?;");
        clear_stmt->BindInt64(1, buyer_id);
        clear_stmt->Step();

        tx->Commit();

        // Retrieve created order
        auto fetch_stmt = db_->Prepare(
            "SELECT o.id, o.buyer_id, o.status, o.total_amount_cents, o.created_at, "
            "u.name as buyer_name, u.email as buyer_email "
            "FROM orders o "
            "JOIN users u ON o.buyer_id = u.id "
            "WHERE o.id = ?;"
        );
        fetch_stmt->BindInt64(1, order_id);
        if (fetch_stmt->Step() == SQLITE_ROW) {
            model::Order o = RowToOrder(fetch_stmt->Get());
            PopulateOrderItems(o);
            return o;
        }

        model::Order fallback;
        fallback.id = order_id;
        fallback.buyer_id = buyer_id;
        fallback.status = model::OrderStatus::kConfirmed;
        fallback.total_amount = model::Money(total_cents);
        return fallback;
    }

    bool UpdateStatus(int64_t order_id, model::OrderStatus status) override {
        std::lock_guard<std::mutex> lock(db_->GetMutex());
        auto stmt = db_->Prepare("UPDATE orders SET status = ? WHERE id = ?;");
        stmt->BindText(1, model::OrderStatusToString(status));
        stmt->BindInt64(2, order_id);
        return stmt->Step() == SQLITE_DONE;
    }

    bool HasBuyerPurchasedProduct(int64_t buyer_id, int64_t product_id) override {
        std::lock_guard<std::mutex> lock(db_->GetMutex());
        auto stmt = db_->Prepare(
            "SELECT 1 FROM orders o "
            "JOIN order_items oi ON o.id = oi.order_id "
            "WHERE o.buyer_id = ? AND oi.product_id = ? AND o.status IN ('CONFIRMED', 'SHIPPED', 'DELIVERED') "
            "LIMIT 1;"
        );
        stmt->BindInt64(1, buyer_id);
        stmt->BindInt64(2, product_id);
        return stmt->Step() == SQLITE_ROW;
    }

    int64_t Count() override {
        std::lock_guard<std::mutex> lock(db_->GetMutex());
        auto stmt = db_->Prepare("SELECT COUNT(*) FROM orders;");
        if (stmt->Step() == SQLITE_ROW) {
            return sqlite3_column_int64(stmt->Get(), 0);
        }
        return 0;
    }

    int64_t TotalGrossMerchandiseValueCents() override {
        std::lock_guard<std::mutex> lock(db_->GetMutex());
        auto stmt = db_->Prepare("SELECT COALESCE(SUM(total_amount_cents), 0) FROM orders WHERE status != 'CANCELLED';");
        if (stmt->Step() == SQLITE_ROW) {
            return sqlite3_column_int64(stmt->Get(), 0);
        }
        return 0;
    }

    int64_t SellerRevenueCents(int64_t seller_id) override {
        std::lock_guard<std::mutex> lock(db_->GetMutex());
        auto stmt = db_->Prepare(
            "SELECT COALESCE(SUM(oi.quantity * oi.unit_price_cents), 0) "
            "FROM order_items oi "
            "JOIN orders o ON oi.order_id = o.id "
            "JOIN products p ON oi.product_id = p.id "
            "WHERE p.seller_id = ? AND o.status != 'CANCELLED';"
        );
        stmt->BindInt64(1, seller_id);
        if (stmt->Step() == SQLITE_ROW) {
            return sqlite3_column_int64(stmt->Get(), 0);
        }
        return 0;
    }

    int64_t SellerOrderCount(int64_t seller_id) override {
        std::lock_guard<std::mutex> lock(db_->GetMutex());
        auto stmt = db_->Prepare(
            "SELECT COUNT(DISTINCT o.id) "
            "FROM orders o "
            "JOIN order_items oi ON o.id = oi.order_id "
            "JOIN products p ON oi.product_id = p.id "
            "WHERE p.seller_id = ?;"
        );
        stmt->BindInt64(1, seller_id);
        if (stmt->Step() == SQLITE_ROW) {
            return sqlite3_column_int64(stmt->Get(), 0);
        }
        return 0;
    }

private:
    void PopulateOrderItems(model::Order& order) {
        auto stmt = db_->Prepare(
            "SELECT oi.id, oi.order_id, oi.product_id, oi.quantity, oi.unit_price_cents, "
            "p.name as product_name, p.image_url as product_image_url, p.seller_id "
            "FROM order_items oi "
            "JOIN products p ON oi.product_id = p.id "
            "WHERE oi.order_id = ?;"
        );
        stmt->BindInt64(1, order.id);

        while (stmt->Step() == SQLITE_ROW) {
            model::OrderItem item;
            item.id = sqlite3_column_int64(stmt->Get(), 0);
            item.order_id = sqlite3_column_int64(stmt->Get(), 1);
            item.product_id = sqlite3_column_int64(stmt->Get(), 2);
            item.quantity = sqlite3_column_int(stmt->Get(), 3);
            item.unit_price = model::Money(sqlite3_column_int64(stmt->Get(), 4));
            item.product_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt->Get(), 5));
            const unsigned char* img = sqlite3_column_text(stmt->Get(), 6);
            item.product_image_url = img ? reinterpret_cast<const char*>(img) : "";
            item.seller_id = sqlite3_column_int64(stmt->Get(), 7);
            order.items.push_back(item);
        }
    }

    static model::Order RowToOrder(sqlite3_stmt* s) {
        model::Order o;
        o.id = sqlite3_column_int64(s, 0);
        o.buyer_id = sqlite3_column_int64(s, 1);
        o.status = model::StringToOrderStatus(reinterpret_cast<const char*>(sqlite3_column_text(s, 2)));
        o.total_amount = model::Money(sqlite3_column_int64(s, 3));
        const unsigned char* created_txt = sqlite3_column_text(s, 4);
        o.created_at = created_txt ? reinterpret_cast<const char*>(created_txt) : "";
        const unsigned char* bname = sqlite3_column_text(s, 5);
        o.buyer_name = bname ? reinterpret_cast<const char*>(bname) : "";
        const unsigned char* bemail = sqlite3_column_text(s, 6);
        o.buyer_email = bemail ? reinterpret_cast<const char*>(bemail) : "";
        return o;
    }

    std::shared_ptr<SqliteDatabase> db_;
};

} // namespace dd::ddmart::repository
