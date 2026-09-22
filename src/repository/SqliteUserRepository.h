#pragma once

#include "repository/IUserRepository.h"
#include "repository/SqliteDatabase.h"

namespace dd::ddmart::repository {

class SqliteUserRepository : public IUserRepository {
public:
    explicit SqliteUserRepository(std::shared_ptr<SqliteDatabase> db)
        : db_(std::move(db)) {}

    std::optional<model::User> FindById(int64_t id) override {
        std::lock_guard<std::mutex> lock(db_->GetMutex());
        auto stmt = db_->Prepare("SELECT id, name, email, password_hash, role, created_at FROM users WHERE id = ?;");
        stmt->BindInt64(1, id);

        if (stmt->Step() == SQLITE_ROW) {
            return RowToUser(stmt->Get());
        }
        return std::nullopt;
    }

    std::optional<model::User> FindByEmail(const std::string& email) override {
        std::lock_guard<std::mutex> lock(db_->GetMutex());
        auto stmt = db_->Prepare("SELECT id, name, email, password_hash, role, created_at FROM users WHERE LOWER(email) = LOWER(?);");
        stmt->BindText(1, email);

        if (stmt->Step() == SQLITE_ROW) {
            return RowToUser(stmt->Get());
        }
        return std::nullopt;
    }

    model::User Create(const model::User& user) override {
        std::lock_guard<std::mutex> lock(db_->GetMutex());
        auto stmt = db_->Prepare(
            "INSERT INTO users (name, email, password_hash, role) VALUES (?, ?, ?, ?);"
        );
        stmt->BindText(1, user.name);
        stmt->BindText(2, user.email);
        stmt->BindText(3, user.password_hash);
        stmt->BindText(4, model::UserRoleToString(user.role));

        if (stmt->Step() != SQLITE_DONE) {
            throw std::runtime_error("Failed to insert user");
        }

        int64_t new_id = db_->GetLastInsertRowId();
        auto fetch_stmt = db_->Prepare("SELECT id, name, email, password_hash, role, created_at FROM users WHERE id = ?;");
        fetch_stmt->BindInt64(1, new_id);
        if (fetch_stmt->Step() == SQLITE_ROW) {
            return RowToUser(fetch_stmt->Get());
        }
        model::User res = user;
        res.id = new_id;
        return res;
    }

    std::vector<model::User> FindAll() override {
        std::lock_guard<std::mutex> lock(db_->GetMutex());
        std::vector<model::User> users;
        auto stmt = db_->Prepare("SELECT id, name, email, password_hash, role, created_at FROM users ORDER BY id ASC;");
        while (stmt->Step() == SQLITE_ROW) {
            users.push_back(RowToUser(stmt->Get()));
        }
        return users;
    }

    int64_t Count() override {
        std::lock_guard<std::mutex> lock(db_->GetMutex());
        auto stmt = db_->Prepare("SELECT COUNT(*) FROM users;");
        if (stmt->Step() == SQLITE_ROW) {
            return sqlite3_column_int64(stmt->Get(), 0);
        }
        return 0;
    }

private:
    static model::User RowToUser(sqlite3_stmt* s) {
        model::User u;
        u.id = sqlite3_column_int64(s, 0);
        u.name = reinterpret_cast<const char*>(sqlite3_column_text(s, 1));
        u.email = reinterpret_cast<const char*>(sqlite3_column_text(s, 2));
        u.password_hash = reinterpret_cast<const char*>(sqlite3_column_text(s, 3));
        u.role = model::StringToUserRole(reinterpret_cast<const char*>(sqlite3_column_text(s, 4)));
        const unsigned char* created_at_txt = sqlite3_column_text(s, 5);
        u.created_at = created_at_txt ? reinterpret_cast<const char*>(created_at_txt) : "";
        return u;
    }

    std::shared_ptr<SqliteDatabase> db_;
};

} // namespace dd::ddmart::repository
