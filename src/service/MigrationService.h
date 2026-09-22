#pragma once

#include <memory>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <iostream>
#include "repository/SqliteDatabase.h"

namespace dd::ddmart::service {

class MigrationService {
public:
    explicit MigrationService(std::shared_ptr<repository::SqliteDatabase> db)
        : db_(std::move(db)) {}

    void RunMigrations(const std::string& migrations_dir) {
        std::cout << ">>> Running database migrations from: " << migrations_dir << std::endl;
        
        // Ensure tracking table exists
        db_->ExecuteScript(
            "CREATE TABLE IF NOT EXISTS schema_migrations ("
            "    version TEXT PRIMARY KEY,"
            "    applied_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
            ");"
        );

        if (!std::filesystem::exists(migrations_dir)) {
            std::cerr << ">>> Migrations directory does not exist: " << migrations_dir << std::endl;
            return;
        }

        std::vector<std::filesystem::path> files;
        for (const auto& entry : std::filesystem::directory_iterator(migrations_dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".sql") {
                files.push_back(entry.path());
            }
        }

        std::sort(files.begin(), files.end());

        for (const auto& f : files) {
            std::string filename = f.filename().string();
            
            // Check if already applied
            auto check_stmt = db_->Prepare("SELECT 1 FROM schema_migrations WHERE version = ?;");
            check_stmt->BindText(1, filename);
            if (check_stmt->Step() == SQLITE_ROW) {
                // Already applied, skip
                continue;
            }

            std::cout << ">>> Applying migration: " << filename << std::endl;
            std::ifstream file(f);
            if (!file.is_open()) {
                throw std::runtime_error("Cannot open migration file: " + f.string());
            }

            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string sql = buffer.str();

            // Apply migration and record version in atomic transaction
            auto tx = db_->BeginTransaction();
            char* errmsg = nullptr;
            int rc = sqlite3_exec(db_->GetRawHandle(), sql.c_str(), nullptr, nullptr, &errmsg);
            if (rc != SQLITE_OK) {
                std::string err = errmsg ? errmsg : "Error running migration SQL";
                sqlite3_free(errmsg);
                throw std::runtime_error("Migration failed on " + filename + ": " + err);
            }

            auto record_stmt = db_->Prepare("INSERT INTO schema_migrations (version) VALUES (?);");
            record_stmt->BindText(1, filename);
            if (record_stmt->Step() != SQLITE_DONE) {
                throw std::runtime_error("Failed to record migration version for " + filename);
            }

            tx->Commit();
            std::cout << ">>> Successfully applied migration: " << filename << std::endl;
        }
    }

    void ApplySeedDataIfEmpty(const std::string& seed_file) {
        if (!std::filesystem::exists(seed_file)) {
            return;
        }

        auto count_stmt = db_->Prepare("SELECT COUNT(*) FROM users;");
        if (count_stmt->Step() == SQLITE_ROW && sqlite3_column_int64(count_stmt->Get(), 0) > 0) {
            std::cout << ">>> Database already seeded. Skipping seed data." << std::endl;
            return;
        }

        std::cout << ">>> Seeding initial data from: " << seed_file << std::endl;
        std::ifstream file(seed_file);
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string sql = buffer.str();
            try {
                db_->ExecuteScript(sql);
                std::cout << ">>> Database seeded successfully." << std::endl;
            } catch (const std::exception& e) {
                std::cerr << ">>> Warning: error during seed data insertion: " << e.what() << std::endl;
            }
        }
    }

private:
    std::shared_ptr<repository::SqliteDatabase> db_;
};

} // namespace dd::ddmart::service
