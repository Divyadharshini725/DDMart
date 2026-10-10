#include <iostream>
#include <memory>
#include <cstdlib>
#include <filesystem>
#include <spdlog/spdlog.h>

#include "repository/SqliteDatabase.h"
#include "repository/MysqlDatabase.h"
#include "repository/SqliteUserRepository.h"
#include "repository/SqliteProductRepository.h"
#include "repository/SqliteCartRepository.h"
#include "repository/SqliteOrderRepository.h"
#include "repository/SqliteReviewRepository.h"

#include "service/UserService.h"
#include "service/ProductService.h"
#include "service/CartService.h"
#include "service/OrderService.h"
#include "service/ReviewService.h"
#include "service/MigrationService.h"
#include "service/ChatService.h"
#include "service/ChatProviderFactory.h"

#include "util/PasswordUtil.h"
#include "controller/HttpServer.h"

int main(int argc, char* argv[]) {
    std::cout << ">>> Initializing DDMart Native C++ Marketplace..." << std::endl;

    // 1. Initialize Cryptographic Subsystem (libsodium Argon2id)
    dd::ddmart::util::PasswordUtil::Init();

    // 2. Resolve database path & port
    int port = 8080;
    const char* env_port = std::getenv("PORT");
    if (env_port != nullptr && std::string(env_port).length() > 0) {
        port = std::atoi(env_port);
    }

    std::string db_path = "ddmart.db";
    const char* env_db = std::getenv("DB_PATH");
    if (env_db != nullptr) {
        db_path = env_db;
    }

    std::string db_type = "sqlite";
    if (const char* env_type = std::getenv("DB_TYPE")) {
        db_type = env_type;
    }

    if (db_type == "mysql") {
        std::cout << ">>> Initializing MySQL Database subsystem (DB_TYPE=mysql)..." << std::endl;
        try {
            auto mysql_db = std::make_shared<dd::ddmart::repository::MysqlDatabase>();
            if (mysql_db->IsConnected()) {
                std::cout << ">>> MySQL subsystem online." << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << ">>> MySQL Connection Notice: " << e.what() << " | Running core catalog on SQLite engine." << std::endl;
        }
    }

    // 3. Connect to Database (RAII Connection)
    auto db = std::make_shared<dd::ddmart::repository::SqliteDatabase>(db_path);

    // 4. Run Migrations and Seed Data (Section 12)
    std::string migrations_dir = "db/migrations";
    if (!std::filesystem::exists(migrations_dir) && std::filesystem::exists("../db/migrations")) {
        migrations_dir = "../db/migrations";
    }

    std::string seed_file = "db/seed.sql";
    if (!std::filesystem::exists(seed_file) && std::filesystem::exists("../db/seed.sql")) {
        seed_file = "../db/seed.sql";
    }

    auto migration_service = std::make_unique<dd::ddmart::service::MigrationService>(db);
    try {
        migration_service->RunMigrations(migrations_dir);
        migration_service->ApplySeedDataIfEmpty(seed_file);
    } catch (const std::exception& e) {
        std::cerr << ">>> Migration/Seed Error: " << e.what() << std::endl;
    }

    // 5. Dependency Injection: Repositories (SOLID - D: Inversion of Control)
    auto user_repo = std::make_shared<dd::ddmart::repository::SqliteUserRepository>(db);
    auto product_repo = std::make_shared<dd::ddmart::repository::SqliteProductRepository>(db);
    auto cart_repo = std::make_shared<dd::ddmart::repository::SqliteCartRepository>(db);
    auto order_repo = std::make_shared<dd::ddmart::repository::SqliteOrderRepository>(db);
    auto review_repo = std::make_shared<dd::ddmart::repository::SqliteReviewRepository>(db);

    // 6. Dependency Injection: Services (Pure Business Rules)
    auto user_service = std::make_shared<dd::ddmart::service::UserService>(user_repo);
    auto product_service = std::make_shared<dd::ddmart::service::ProductService>(product_repo);
    auto cart_service = std::make_shared<dd::ddmart::service::CartService>(cart_repo, product_repo);
    auto order_service = std::make_shared<dd::ddmart::service::OrderService>(order_repo, cart_repo, product_repo);
    auto review_service = std::make_shared<dd::ddmart::service::ReviewService>(review_repo, order_repo, product_repo);

    // AI Chatbot Service (Phase 3: Strategy & Factory Patterns)
    auto chat_provider = std::shared_ptr<dd::ddmart::service::ChatProvider>(
        dd::ddmart::service::ChatProviderFactory::Create()
    );
    auto chat_service = std::make_shared<dd::ddmart::service::ChatService>(chat_provider, product_repo);

    // 7. Resolve Public Web Assets Directory
    std::string public_dir = "public";
    if (!std::filesystem::exists(public_dir) && std::filesystem::exists("../public")) {
        public_dir = "../public";
    }

    // 8. Start HTTP Server
    dd::ddmart::controller::HttpServer server(
        user_service, product_service, cart_service, order_service, review_service, db, chat_service
    );
    server.SetupRoutes(public_dir);

    server.Listen("0.0.0.0", port);

    return 0;
}
