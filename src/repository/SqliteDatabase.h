#pragma once

#include <sqlite3.h>
#include <string>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <iostream>

namespace dd::ddmart::repository {

class SqliteStatement {
public:
    SqliteStatement(sqlite3* db, const std::string& sql) {
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt_, nullptr);
        if (rc != SQLITE_OK) {
            throw std::runtime_error(std::string("Failed to prepare statement: ") + sqlite3_errmsg(db) + " | SQL: " + sql);
        }
    }

    ~SqliteStatement() {
        if (stmt_) {
            sqlite3_finalize(stmt_);
        }
    }

    SqliteStatement(const SqliteStatement&) = delete;
    SqliteStatement& operator=(const SqliteStatement&) = delete;
    SqliteStatement(SqliteStatement&& other) noexcept : stmt_(other.stmt_) { other.stmt_ = nullptr; }
    SqliteStatement& operator=(SqliteStatement&& other) noexcept {
        if (this != &other) {
            if (stmt_) sqlite3_finalize(stmt_);
            stmt_ = other.stmt_;
            other.stmt_ = nullptr;
        }
        return *this;
    }

    [[nodiscard]] sqlite3_stmt* Get() const noexcept { return stmt_; }

    void BindInt64(int index, int64_t val) {
        sqlite3_bind_int64(stmt_, index, val);
    }

    void BindInt(int index, int32_t val) {
        sqlite3_bind_int(stmt_, index, val);
    }

    void BindText(int index, const std::string& val) {
        sqlite3_bind_text(stmt_, index, val.c_str(), -1, SQLITE_TRANSIENT);
    }

    void BindNull(int index) {
        sqlite3_bind_null(stmt_, index);
    }

    int Step() {
        return sqlite3_step(stmt_);
    }

    void Reset() {
        sqlite3_reset(stmt_);
        sqlite3_clear_bindings(stmt_);
    }

private:
    sqlite3_stmt* stmt_{nullptr};
};

class SqliteTransaction {
public:
    explicit SqliteTransaction(sqlite3* db) : db_(db) {
        char* err = nullptr;
        if (sqlite3_exec(db_, "BEGIN TRANSACTION;", nullptr, nullptr, &err) != SQLITE_OK) {
            std::string msg = err ? err : "Transaction begin failed";
            sqlite3_free(err);
            throw std::runtime_error(msg);
        }
    }

    ~SqliteTransaction() {
        if (!committed_ && db_) {
            char* err = nullptr;
            sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, &err);
            if (err) sqlite3_free(err);
        }
    }

    void Commit() {
        if (!committed_ && db_) {
            char* err = nullptr;
            if (sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, &err) != SQLITE_OK) {
                std::string msg = err ? err : "Transaction commit failed";
                sqlite3_free(err);
                throw std::runtime_error(msg);
            }
            committed_ = true;
        }
    }

    void Rollback() {
        if (!committed_ && db_) {
            char* err = nullptr;
            sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, &err);
            if (err) sqlite3_free(err);
            committed_ = true;
        }
    }

private:
    sqlite3* db_{nullptr};
    bool committed_{false};
};

class SqliteDatabase {
public:
    explicit SqliteDatabase(const std::string& db_path) : db_path_(db_path) {
        int rc = sqlite3_open(db_path.c_str(), &db_);
        if (rc != SQLITE_OK) {
            std::string err = sqlite3_errmsg(db_);
            if (db_) sqlite3_close(db_);
            throw std::runtime_error("Cannot open SQLite database at " + db_path + ": " + err);
        }
        // Enable WAL mode and foreign keys
        char* errmsg = nullptr;
        sqlite3_exec(db_, "PRAGMA foreign_keys = ON; PRAGMA journal_mode = WAL;", nullptr, nullptr, &errmsg);
        if (errmsg) sqlite3_free(errmsg);
    }

    ~SqliteDatabase() {
        if (db_) {
            sqlite3_close(db_);
        }
    }

    SqliteDatabase(const SqliteDatabase&) = delete;
    SqliteDatabase& operator=(const SqliteDatabase&) = delete;

    [[nodiscard]] sqlite3* GetRawHandle() const noexcept { return db_; }
    [[nodiscard]] std::unique_ptr<SqliteStatement> Prepare(const std::string& sql) const {
        return std::make_unique<SqliteStatement>(db_, sql);
    }

    [[nodiscard]] std::unique_ptr<SqliteTransaction> BeginTransaction() const {
        return std::make_unique<SqliteTransaction>(db_);
    }

    [[nodiscard]] std::mutex& GetMutex() { return db_mutex_; }

    void ExecuteScript(const std::string& sql) {
        std::lock_guard<std::mutex> lock(db_mutex_);
        char* errmsg = nullptr;
        int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errmsg);
        if (rc != SQLITE_OK) {
            std::string msg = errmsg ? errmsg : "SQL execution error";
            sqlite3_free(errmsg);
            throw std::runtime_error(msg);
        }
    }

    [[nodiscard]] int64_t GetLastInsertRowId() const noexcept {
        return sqlite3_last_insert_rowid(db_);
    }

private:
    std::string db_path_;
    sqlite3* db_{nullptr};
    std::mutex db_mutex_;
};

} // namespace dd::ddmart::repository
