#include <gtest/gtest.h>
#include "repository/SqliteDatabase.h"
#include "repository/SqliteUserRepository.h"
#include "repository/SqliteProductRepository.h"
#include "repository/SqliteCartRepository.h"
#include "repository/SqliteOrderRepository.h"
#include "repository/SqliteReviewRepository.h"

using namespace dd::ddmart;

class SqliteRepositoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        db = std::make_shared<repository::SqliteDatabase>(":memory:");
        
        // Execute DDMart base schema in memory
        db->ExecuteScript(
            "CREATE TABLE users ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    name TEXT NOT NULL,"
            "    email TEXT NOT NULL UNIQUE,"
            "    password_hash TEXT NOT NULL,"
            "    role TEXT NOT NULL CHECK(role IN ('BUYER', 'SELLER', 'ADMIN')),"
            "    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
            ");"
            "CREATE TABLE products ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    seller_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,"
            "    name TEXT NOT NULL,"
            "    description TEXT,"
            "    price_cents INTEGER NOT NULL CHECK(price_cents >= 0),"
            "    stock_qty INTEGER NOT NULL CHECK(stock_qty >= 0),"
            "    category TEXT NOT NULL,"
            "    image_url TEXT,"
            "    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
            ");"
            "CREATE TABLE orders ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    buyer_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,"
            "    status TEXT NOT NULL CHECK(status IN ('PENDING', 'CONFIRMED', 'SHIPPED', 'DELIVERED', 'CANCELLED')),"
            "    total_amount_cents INTEGER NOT NULL CHECK(total_amount_cents >= 0),"
            "    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
            ");"
            "CREATE TABLE order_items ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    order_id INTEGER NOT NULL REFERENCES orders(id) ON DELETE CASCADE,"
            "    product_id INTEGER NOT NULL REFERENCES products(id) ON DELETE CASCADE,"
            "    quantity INTEGER NOT NULL CHECK(quantity > 0),"
            "    unit_price_cents INTEGER NOT NULL CHECK(unit_price_cents >= 0)"
            ");"
            "CREATE TABLE cart_items ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,"
            "    product_id INTEGER NOT NULL REFERENCES products(id) ON DELETE CASCADE,"
            "    quantity INTEGER NOT NULL CHECK(quantity > 0),"
            "    UNIQUE(user_id, product_id)"
            ");"
            "CREATE TABLE reviews ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    product_id INTEGER NOT NULL REFERENCES products(id) ON DELETE CASCADE,"
            "    user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,"
            "    rating INTEGER NOT NULL CHECK(rating >= 1 AND rating <= 5),"
            "    comment TEXT,"
            "    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP"
            ");"
        );

        user_repo = std::make_shared<repository::SqliteUserRepository>(db);
        product_repo = std::make_shared<repository::SqliteProductRepository>(db);
        cart_repo = std::make_shared<repository::SqliteCartRepository>(db);
        order_repo = std::make_shared<repository::SqliteOrderRepository>(db);
        review_repo = std::make_shared<repository::SqliteReviewRepository>(db);
    }

    std::shared_ptr<repository::SqliteDatabase> db;
    std::shared_ptr<repository::SqliteUserRepository> user_repo;
    std::shared_ptr<repository::SqliteProductRepository> product_repo;
    std::shared_ptr<repository::SqliteCartRepository> cart_repo;
    std::shared_ptr<repository::SqliteOrderRepository> order_repo;
    std::shared_ptr<repository::SqliteReviewRepository> review_repo;
};

TEST_F(SqliteRepositoryTest, CreatesAndFindsUser) {
    model::User u;
    u.name = "Seller Dave";
    u.email = "dave@seller.com";
    u.password_hash = "mock_hash";
    u.role = model::UserRole::kSeller;

    model::User created = user_repo->Create(u);
    EXPECT_GT(created.id, 0);

    auto found = user_repo->FindByEmail("dave@seller.com");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->name, "Seller Dave");
    EXPECT_EQ(found->role, model::UserRole::kSeller);
}

TEST_F(SqliteRepositoryTest, CreatesAndFiltersProducts) {
    model::User seller;
    seller.name = "Tech Store";
    seller.email = "tech@store.com";
    seller.password_hash = "hash";
    seller.role = model::UserRole::kSeller;
    seller = user_repo->Create(seller);

    model::Product p1;
    p1.seller_id = seller.id;
    p1.name = "Gaming Laptop RTX 4080";
    p1.description = "Top-tier performance";
    p1.price = model::Money(199999);
    p1.stock_qty = 5;
    p1.category = "Electronics";
    p1 = product_repo->Create(p1);

    auto retrieved = product_repo->FindById(p1.id);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->name, "Gaming Laptop RTX 4080");
    EXPECT_EQ(retrieved->price.GetCents(), 199999);

    repository::ProductFilter filter;
    filter.keyword = "Gaming";
    auto results = product_repo->FindByFilter(filter);
    EXPECT_EQ(results.size(), 1);

    // Stock decrement test
    EXPECT_TRUE(product_repo->DecrementStock(p1.id, 2));
    auto after_dec = product_repo->FindById(p1.id);
    EXPECT_EQ(after_dec->stock_qty, 3);
}

TEST_F(SqliteRepositoryTest, CartAndAtomicOrderCheckout) {
    model::User seller;
    seller.name = "Bookstore";
    seller.email = "books@mart.com";
    seller.password_hash = "hash";
    seller.role = model::UserRole::kSeller;
    seller = user_repo->Create(seller);

    model::User buyer;
    buyer.name = "Student Reader";
    buyer.email = "reader@student.com";
    buyer.password_hash = "hash";
    buyer.role = model::UserRole::kBuyer;
    buyer = user_repo->Create(buyer);

    model::Product p;
    p.seller_id = seller.id;
    p.name = "Modern C++ Programming Guide";
    p.description = "Complete textbook";
    p.price = model::Money(4500);
    p.stock_qty = 10;
    p.category = "Books";
    p = product_repo->Create(p);

    // Cart operations
    cart_repo->AddOrUpdate(buyer.id, p.id, 2);
    auto cart = cart_repo->FindByUserId(buyer.id);
    ASSERT_EQ(cart.size(), 1);
    EXPECT_EQ(cart[0].quantity, 2);

    // Create Order with Items
    std::vector<model::OrderItem> items;
    model::OrderItem oi;
    oi.product_id = p.id;
    oi.quantity = 2;
    oi.unit_price = p.price;
    items.push_back(oi);

    model::Order order = order_repo->CreateOrderWithItems(buyer.id, items);
    EXPECT_GT(order.id, 0);
    EXPECT_EQ(order.total_amount.GetCents(), 9000); // 2 * 4500
    EXPECT_EQ(order.status, model::OrderStatus::kConfirmed);

    // Stock must be decremented from 10 down to 8
    auto p_after = product_repo->FindById(p.id);
    EXPECT_EQ(p_after->stock_qty, 8);

    // Buyer has purchased check
    EXPECT_TRUE(order_repo->HasBuyerPurchasedProduct(buyer.id, p.id));
}
