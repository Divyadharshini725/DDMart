#pragma once

#include "repository/IReviewRepository.h"
#include "repository/SqliteDatabase.h"

namespace dd::ddmart::repository {

class SqliteReviewRepository : public IReviewRepository {
public:
    explicit SqliteReviewRepository(std::shared_ptr<SqliteDatabase> db)
        : db_(std::move(db)) {}

    std::vector<model::Review> FindByProductId(int64_t product_id) override {
        std::lock_guard<std::mutex> lock(db_->GetMutex());
        auto stmt = db_->Prepare(
            "SELECT r.id, r.product_id, r.user_id, r.rating, r.comment, r.created_at, "
            "u.name as user_name, p.name as product_name "
            "FROM reviews r "
            "JOIN users u ON r.user_id = u.id "
            "JOIN products p ON r.product_id = p.id "
            "WHERE r.product_id = ? "
            "ORDER BY r.id DESC;"
        );
        stmt->BindInt64(1, product_id);

        std::vector<model::Review> list;
        while (stmt->Step() == SQLITE_ROW) {
            model::Review r;
            r.id = sqlite3_column_int64(stmt->Get(), 0);
            r.product_id = sqlite3_column_int64(stmt->Get(), 1);
            r.user_id = sqlite3_column_int64(stmt->Get(), 2);
            r.rating = sqlite3_column_int(stmt->Get(), 3);
            const unsigned char* comment_txt = sqlite3_column_text(stmt->Get(), 4);
            r.comment = comment_txt ? reinterpret_cast<const char*>(comment_txt) : "";
            const unsigned char* created_txt = sqlite3_column_text(stmt->Get(), 5);
            r.created_at = created_txt ? reinterpret_cast<const char*>(created_txt) : "";
            const unsigned char* uname = sqlite3_column_text(stmt->Get(), 6);
            r.user_name = uname ? reinterpret_cast<const char*>(uname) : "";
            const unsigned char* pname = sqlite3_column_text(stmt->Get(), 7);
            r.product_name = pname ? reinterpret_cast<const char*>(pname) : "";
            list.push_back(r);
        }
        return list;
    }

    std::optional<model::Review> FindByProductAndUser(int64_t product_id, int64_t user_id) override {
        std::lock_guard<std::mutex> lock(db_->GetMutex());
        auto stmt = db_->Prepare(
            "SELECT r.id, r.product_id, r.user_id, r.rating, r.comment, r.created_at, "
            "u.name as user_name, p.name as product_name "
            "FROM reviews r "
            "JOIN users u ON r.user_id = u.id "
            "JOIN products p ON r.product_id = p.id "
            "WHERE r.product_id = ? AND r.user_id = ?;"
        );
        stmt->BindInt64(1, product_id);
        stmt->BindInt64(2, user_id);

        if (stmt->Step() == SQLITE_ROW) {
            model::Review r;
            r.id = sqlite3_column_int64(stmt->Get(), 0);
            r.product_id = sqlite3_column_int64(stmt->Get(), 1);
            r.user_id = sqlite3_column_int64(stmt->Get(), 2);
            r.rating = sqlite3_column_int(stmt->Get(), 3);
            const unsigned char* comment_txt = sqlite3_column_text(stmt->Get(), 4);
            r.comment = comment_txt ? reinterpret_cast<const char*>(comment_txt) : "";
            const unsigned char* created_txt = sqlite3_column_text(stmt->Get(), 5);
            r.created_at = created_txt ? reinterpret_cast<const char*>(created_txt) : "";
            const unsigned char* uname = sqlite3_column_text(stmt->Get(), 6);
            r.user_name = uname ? reinterpret_cast<const char*>(uname) : "";
            const unsigned char* pname = sqlite3_column_text(stmt->Get(), 7);
            r.product_name = pname ? reinterpret_cast<const char*>(pname) : "";
            return r;
        }
        return std::nullopt;
    }

    model::Review Create(const model::Review& review) override {
        std::lock_guard<std::mutex> lock(db_->GetMutex());
        auto stmt = db_->Prepare(
            "INSERT INTO reviews (product_id, user_id, rating, comment) VALUES (?, ?, ?, ?);"
        );
        stmt->BindInt64(1, review.product_id);
        stmt->BindInt64(2, review.user_id);
        stmt->BindInt(3, review.rating);
        stmt->BindText(4, review.comment);

        if (stmt->Step() != SQLITE_DONE) {
            throw std::runtime_error("Failed to insert review");
        }
        int64_t new_id = db_->GetLastInsertRowId();
        model::Review res = review;
        res.id = new_id;
        return res;
    }

    double GetAverageRating(int64_t product_id) override {
        std::lock_guard<std::mutex> lock(db_->GetMutex());
        auto stmt = db_->Prepare("SELECT COALESCE(AVG(rating), 0.0) FROM reviews WHERE product_id = ?;");
        stmt->BindInt64(1, product_id);
        if (stmt->Step() == SQLITE_ROW) {
            return sqlite3_column_double(stmt->Get(), 0);
        }
        return 0.0;
    }

    int32_t GetReviewCount(int64_t product_id) override {
        std::lock_guard<std::mutex> lock(db_->GetMutex());
        auto stmt = db_->Prepare("SELECT COUNT(*) FROM reviews WHERE product_id = ?;");
        stmt->BindInt64(1, product_id);
        if (stmt->Step() == SQLITE_ROW) {
            return sqlite3_column_int(stmt->Get(), 0);
        }
        return 0;
    }

private:
    std::shared_ptr<SqliteDatabase> db_;
};

} // namespace dd::ddmart::repository
