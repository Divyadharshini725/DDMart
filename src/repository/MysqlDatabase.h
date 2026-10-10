#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <cstdlib>

#if defined(DDMART_HAS_MYSQL) && DDMART_HAS_MYSQL == 1
#if __has_include(<mysql/mysql.h>)
#include <mysql/mysql.h>
#elif __has_include(<mariadb/mysql.h>)
#include <mariadb/mysql.h>
#elif __has_include(<mysql.h>)
#include <mysql.h>
#endif
#endif

namespace dd::ddmart::repository {

/**
 * @brief Thread-safe MySQL database connection wrapper.
 * Connects to a live MySQL / MariaDB server via standard environment variables.
 */
class MysqlDatabase {
public:
    struct Config {
        std::string host = "127.0.0.1";
        int port = 3306;
        std::string user = "root";
        std::string password = "";
        std::string database = "ddmart";

        static Config FromEnvironment() {
            Config cfg;
            if (const char* v = std::getenv("MYSQL_HOST")) cfg.host = v;
            if (const char* v = std::getenv("MYSQL_PORT")) cfg.port = std::atoi(v);
            if (const char* v = std::getenv("MYSQL_USER")) cfg.user = v;
            if (const char* v = std::getenv("MYSQL_PASSWORD")) cfg.password = v;
            if (const char* v = std::getenv("MYSQL_DATABASE")) cfg.database = v;
            return cfg;
        }
    };

    explicit MysqlDatabase(Config config = Config::FromEnvironment())
        : config_(std::move(config)) {
        Connect();
    }

    ~MysqlDatabase() {
        Disconnect();
    }

    MysqlDatabase(const MysqlDatabase&) = delete;
    MysqlDatabase& operator=(const MysqlDatabase&) = delete;

    [[nodiscard]] std::mutex& GetMutex() noexcept { return mutex_; }

    void Connect() {
        std::lock_guard<std::mutex> lock(mutex_);
#if defined(DDMART_HAS_MYSQL) && DDMART_HAS_MYSQL == 1
        conn_ = mysql_init(nullptr);
        if (!conn_) {
            throw std::runtime_error("MySQL initialization failed: out of memory");
        }

        bool reconnect = true;
        mysql_options(conn_, MYSQL_OPT_RECONNECT, &reconnect);

        if (!mysql_real_connect(conn_, config_.host.c_str(), config_.user.c_str(),
                               config_.password.c_str(), config_.database.c_str(),
                               config_.port, nullptr, 0)) {
            std::string err = mysql_error(conn_);
            mysql_close(conn_);
            conn_ = nullptr;
            throw std::runtime_error("MySQL connection failed to " + config_.host + ":" +
                                     std::to_string(config_.port) + " - " + err);
        }

        mysql_set_character_set(conn_, "utf8mb4");
        std::cout << ">>> Connected to MySQL Database [" << config_.database
                  << "] at " << config_.host << ":" << config_.port << std::endl;
#else
        std::cout << ">>> Notice: DDMart is configured with DB_TYPE=mysql, but MySQL C connector library was not compiled in this build." << std::endl;
        std::cout << ">>> Please install libmysqlclient/libmariadb-dev or run with default SQLite." << std::endl;
#endif
    }

    void Disconnect() {
#if defined(DDMART_HAS_MYSQL) && DDMART_HAS_MYSQL == 1
        std::lock_guard<std::mutex> lock(mutex_);
        if (conn_) {
            mysql_close(conn_);
            conn_ = nullptr;
        }
#endif
    }

    void Execute(const std::string& sql) {
        std::lock_guard<std::mutex> lock(mutex_);
#if defined(DDMART_HAS_MYSQL) && DDMART_HAS_MYSQL == 1
        if (!conn_) throw std::runtime_error("MySQL is not connected");
        if (mysql_query(conn_, sql.c_str()) != 0) {
            throw std::runtime_error("MySQL Execute Error: " + std::string(mysql_error(conn_)) + " | SQL: " + sql);
        }
#else
        (void)sql;
        throw std::runtime_error("MySQL C connector is not compiled into this binary.");
#endif
    }

    std::vector<std::vector<std::string>> Query(const std::string& sql) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<std::vector<std::string>> results;
#if defined(DDMART_HAS_MYSQL) && DDMART_HAS_MYSQL == 1
        if (!conn_) throw std::runtime_error("MySQL is not connected");
        if (mysql_query(conn_, sql.c_str()) != 0) {
            throw std::runtime_error("MySQL Query Error: " + std::string(mysql_error(conn_)) + " | SQL: " + sql);
        }

        MYSQL_RES* res = mysql_store_result(conn_);
        if (!res) {
            if (mysql_field_count(conn_) == 0) return results; // Non-SELECT statement
            throw std::runtime_error("MySQL Store Result Error: " + std::string(mysql_error(conn_)));
        }

        int num_fields = mysql_num_fields(res);
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res))) {
            std::vector<std::string> row_data;
            row_data.reserve(num_fields);
            for (int i = 0; i < num_fields; ++i) {
                row_data.push_back(row[i] ? row[i] : "");
            }
            results.push_back(std::move(row_data));
        }
        mysql_free_result(res);
#else
        (void)sql;
        throw std::runtime_error("MySQL C connector is not compiled into this binary.");
#endif
        return results;
    }

    [[nodiscard]] int64_t GetLastInsertId() {
        std::lock_guard<std::mutex> lock(mutex_);
#if defined(DDMART_HAS_MYSQL) && DDMART_HAS_MYSQL == 1
        if (!conn_) return 0;
        return static_cast<int64_t>(mysql_insert_id(conn_));
#else
        return 0;
#endif
    }

    std::string EscapeString(const std::string& str) {
        std::lock_guard<std::mutex> lock(mutex_);
#if defined(DDMART_HAS_MYSQL) && DDMART_HAS_MYSQL == 1
        if (!conn_) return str;
        std::string buffer(str.size() * 2 + 1, '\0');
        unsigned long len = mysql_real_escape_string(conn_, &buffer[0], str.c_str(), str.size());
        buffer.resize(len);
        return buffer;
#else
        // Fallback simple quote escape
        std::string res;
        for (char c : str) {
            if (c == '\'') res += "''";
            else if (c == '\\') res += "\\\\";
            else res += c;
        }
        return res;
#endif
    }

    [[nodiscard]] bool IsConnected() const noexcept {
#if defined(DDMART_HAS_MYSQL) && DDMART_HAS_MYSQL == 1
        return conn_ != nullptr;
#else
        return false;
#endif
    }

private:
    Config config_;
    std::mutex mutex_;
#if defined(DDMART_HAS_MYSQL) && DDMART_HAS_MYSQL == 1
    MYSQL* conn_{nullptr};
#endif
};

} // namespace dd::ddmart::repository
