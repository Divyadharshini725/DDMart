#pragma once

#include "repository/IProductRepository.h"
#include "repository/SqliteDatabase.h"

namespace dd::ddmart::repository {

class SqliteProductRepository : public IProductRepository {
public:
    explicit SqliteProductRepository(std::shared_ptr<SqliteDatabase> db)
        : db_(std::move(db)) {}

    std::optional<model::Product> FindById(int64_t id) override {
        std::lock_guard<std::mutex> lock(db_->GetMutex());
        auto stmt = db_->Prepare(
            "SELECT p.id, p.seller_id, p.name, p.description, p.price_cents, p.stock_qty, "
            "p.category, p.image_url, p.created_at, u.name as seller_name, "
            "COALESCE(AVG(r.rating), 0.0) as avg_rating, COUNT(r.id) as review_count "
            "FROM products p "
            "JOIN users u ON p.seller_id = u.id "
            "LEFT JOIN reviews r ON p.id = r.product_id "
            "WHERE p.id = ? "
            "GROUP BY p.id;"
        );
        stmt->BindInt64(1, id);

        if (stmt->Step() == SQLITE_ROW) {
            return RowToProduct(stmt->Get());
        }
        return std::nullopt;
    }

    std::vector<model::Product> FindByFilter(const ProductFilter& filter) override {
        std::lock_guard<std::mutex> lock(db_->GetMutex());
        std::string sql = 
            "SELECT p.id, p.seller_id, p.name, p.description, p.price_cents, p.stock_qty, "
            "p.category, p.image_url, p.created_at, u.name as seller_name, "
            "COALESCE(AVG(r.rating), 0.0) as avg_rating, COUNT(r.id) as review_count "
            "FROM products p "
            "JOIN users u ON p.seller_id = u.id "
            "LEFT JOIN reviews r ON p.id = r.product_id "
            "WHERE (?1 = '' OR LOWER(p.category) = LOWER(?1)) "
            "  AND (?2 = '' OR LOWER(p.name) LIKE ?3 OR LOWER(p.description) LIKE ?3) "
            "GROUP BY p.id ";

        if (filter.sort_by == "price_asc") {
            sql += "ORDER BY p.price_cents ASC ";
        } else if (filter.sort_by == "price_desc") {
            sql += "ORDER BY p.price_cents DESC ";
        } else if (filter.sort_by == "rating_desc") {
            sql += "ORDER BY avg_rating DESC ";
        } else {
            sql += "ORDER BY p.created_at DESC ";
        }

        sql += "LIMIT ?4 OFFSET ?5;";

        auto stmt = db_->Prepare(sql);
        stmt->BindText(1, filter.category);
        stmt->BindText(2, filter.keyword);
        std::string kw_like = "%" + filter.keyword + "%";
        stmt->BindText(3, kw_like);
        stmt->BindInt(4, filter.limit > 0 ? filter.limit : 20);
        int offset = (filter.page > 0 ? filter.page - 1 : 0) * (filter.limit > 0 ? filter.limit : 20);
        stmt->BindInt(5, offset);

        std::vector<model::Product> list;
        while (stmt->Step() == SQLITE_ROW) {
            list.push_back(RowToProduct(stmt->Get()));
        }
        return list;
    }

    int64_t CountByFilter(const ProductFilter& filter) override {
        std::lock_guard<std::mutex> lock(db_->GetMutex());
        std::string sql = 
            "SELECT COUNT(DISTINCT p.id) FROM products p "
            "WHERE (?1 = '' OR LOWER(p.category) = LOWER(?1)) "
            "  AND (?2 = '' OR LOWER(p.name) LIKE ?3 OR LOWER(p.description) LIKE ?3);";
        auto stmt = db_->Prepare(sql);
        stmt->BindText(1, filter.category);
        stmt->BindText(2, filter.keyword);
        std::string kw_like = "%" + filter.keyword + "%";
        stmt->BindText(3, kw_like);

        if (stmt->Step() == SQLITE_ROW) {
            return sqlite3_column_int64(stmt->Get(), 0);
        }
        return 0;
    }

    std::vector<model::Product> FindBySellerId(int64_t seller_id) override {
        std::lock_guard<std::mutex> lock(db_->GetMutex());
        auto stmt = db_->Prepare(
            "SELECT p.id, p.seller_id, p.name, p.description, p.price_cents, p.stock_qty, "
            "p.category, p.image_url, p.created_at, u.name as seller_name, "
            "COALESCE(AVG(r.rating), 0.0) as avg_rating, COUNT(r.id) as review_count "
            "FROM products p "
            "JOIN users u ON p.seller_id = u.id "
            "LEFT JOIN reviews r ON p.id = r.product_id "
            "WHERE p.seller_id = ? "
            "GROUP BY p.id "
            "ORDER BY p.id DESC;"
        );
        stmt->BindInt64(1, seller_id);

        std::vector<model::Product> list;
        while (stmt->Step() == SQLITE_ROW) {
            list.push_back(RowToProduct(stmt->Get()));
        }
        return list;
    }

    model::Product Create(const model::Product& product) override {
        std::lock_guard<std::mutex> lock(db_->GetMutex());
        auto stmt = db_->Prepare(
            "INSERT INTO products (seller_id, name, description, price_cents, stock_qty, category, image_url) "
            "VALUES (?, ?, ?, ?, ?, ?, ?);"
        );
        stmt->BindInt64(1, product.seller_id);
        stmt->BindText(2, product.name);
        stmt->BindText(3, product.description);
        stmt->BindInt64(4, product.price.GetCents());
        stmt->BindInt(5, product.stock_qty);
        stmt->BindText(6, product.category);
        stmt->BindText(7, product.image_url);

        if (stmt->Step() != SQLITE_DONE) {
            throw std::runtime_error("Failed to insert product");
        }

        int64_t new_id = db_->GetLastInsertRowId();
        lock.~lock_guard();
        auto created = FindById(new_id);
        if (created) return *created;
        model::Product p = product;
        p.id = new_id;
        return p;
    }

    bool Update(const model::Product& product) override {
        std::lock_guard<std::mutex> lock(db_->GetMutex());
        auto stmt = db_->Prepare(
            "UPDATE products SET name = ?, description = ?, price_cents = ?, stock_qty = ?, category = ?, image_url = ? "
            "WHERE id = ? AND seller_id = ?;"
        );
        stmt->BindText(1, product.name);
        stmt->BindText(2, product.description);
        stmt->BindInt64(3, product.price.GetCents());
        stmt->BindInt(4, product.stock_qty);
        stmt->BindText(5, product.category);
        stmt->BindText(6, product.image_url);
        stmt->BindInt64(7, product.id);
        stmt->BindInt64(8, product.seller_id);

        return stmt->Step() == SQLITE_DONE;
    }

    bool Delete(int64_t id) override {
        std::lock_guard<std::mutex> lock(db_->GetMutex());
        auto stmt = db_->Prepare("DELETE FROM products WHERE id = ?;");
        stmt->BindInt64(1, id);
        return stmt->Step() == SQLITE_DONE;
    }

    bool DecrementStock(int64_t product_id, int32_t quantity) override {
        std::lock_guard<std::mutex> lock(db_->GetMutex());
        auto stmt = db_->Prepare("UPDATE products SET stock_qty = stock_qty - ? WHERE id = ? AND stock_qty >= ?;");
        stmt->BindInt(1, quantity);
        stmt->BindInt64(2, product_id);
        stmt->BindInt(3, quantity);
        return (stmt->Step() == SQLITE_DONE) && (sqlite3_changes(db_->GetRawHandle()) > 0);
    }

    int64_t Count() override {
        std::lock_guard<std::mutex> lock(db_->GetMutex());
        auto stmt = db_->Prepare("SELECT COUNT(*) FROM products;");
        if (stmt->Step() == SQLITE_ROW) {
            return sqlite3_column_int64(stmt->Get(), 0);
        }
        return 0;
    }

private:
    static model::Product RowToProduct(sqlite3_stmt* s) {
        model::Product p;
        p.id = sqlite3_column_int64(s, 0);
        p.seller_id = sqlite3_column_int64(s, 1);
        p.name = reinterpret_cast<const char*>(sqlite3_column_text(s, 2));
        const unsigned char* desc = sqlite3_column_text(s, 3);
        p.description = desc ? reinterpret_cast<const char*>(desc) : "";
        p.price = model::Money(sqlite3_column_int64(s, 4));
        p.stock_qty = sqlite3_column_int(s, 5);
        p.category = reinterpret_cast<const char*>(sqlite3_column_text(s, 6));
        const unsigned char* img = sqlite3_column_text(s, 7);
        p.image_url = img ? reinterpret_cast<const char*>(img) : "";
        const unsigned char* cat = sqlite3_column_text(s, 8);
        p.created_at = cat ? reinterpret_cast<const char*>(cat) : "";
        const unsigned char* sname = sqlite3_column_text(s, 9);
        p.seller_name = sname ? reinterpret_cast<const char*>(sname) : "";
        p.average_rating = sqlite3_column_double(s, 10);
        p.review_count = sqlite3_column_int(s, 11);
        return p;
    }

    std::shared_ptr<SqliteDatabase> db_;
};

} // namespace dd::ddmart::repository
