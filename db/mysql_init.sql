-- =====================================================================
-- DDMart MySQL Database Initialization & Schema Script
-- Compatible with MySQL 8.0+, MariaDB 10.5+, phpMyAdmin, MySQL Workbench
-- =====================================================================

CREATE DATABASE IF NOT EXISTS ddmart
    CHARACTER SET utf8mb4
    COLLATE utf8mb4_unicode_ci;

USE ddmart;

-- -------------------------------------------------------------
-- 1. Users Table (RBAC: BUYER, SELLER, ADMIN)
-- -------------------------------------------------------------
CREATE TABLE IF NOT EXISTS users (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    email VARCHAR(255) NOT NULL UNIQUE,
    password_hash VARCHAR(255) NOT NULL,
    role ENUM('BUYER', 'SELLER', 'ADMIN') NOT NULL DEFAULT 'BUYER',
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_users_email (email),
    INDEX idx_users_role (role)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- -------------------------------------------------------------
-- 2. Products Table (Catalog & Inventory)
-- -------------------------------------------------------------
CREATE TABLE IF NOT EXISTS products (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    seller_id BIGINT NOT NULL,
    name VARCHAR(255) NOT NULL,
    description TEXT,
    price_cents BIGINT NOT NULL CHECK (price_cents >= 0),
    stock_qty INT NOT NULL CHECK (stock_qty >= 0),
    category VARCHAR(100) NOT NULL,
    image_url VARCHAR(1024),
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_products_seller_id (seller_id),
    INDEX idx_products_category (category),
    CONSTRAINT fk_products_seller FOREIGN KEY (seller_id) REFERENCES users(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- -------------------------------------------------------------
-- 3. Orders Table (Checkout & Order Status)
-- -------------------------------------------------------------
CREATE TABLE IF NOT EXISTS orders (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    buyer_id BIGINT NOT NULL,
    status ENUM('PENDING', 'CONFIRMED', 'SHIPPED', 'DELIVERED', 'CANCELLED') NOT NULL DEFAULT 'PENDING',
    total_amount_cents BIGINT NOT NULL CHECK (total_amount_cents >= 0),
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_orders_buyer_id (buyer_id),
    INDEX idx_orders_status (status),
    CONSTRAINT fk_orders_buyer FOREIGN KEY (buyer_id) REFERENCES users(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- -------------------------------------------------------------
-- 4. Order Items Table (Purchased Line Items)
-- -------------------------------------------------------------
CREATE TABLE IF NOT EXISTS order_items (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    order_id BIGINT NOT NULL,
    product_id BIGINT NOT NULL,
    quantity INT NOT NULL CHECK (quantity > 0),
    unit_price_cents BIGINT NOT NULL CHECK (unit_price_cents >= 0),
    INDEX idx_order_items_order_id (order_id),
    INDEX idx_order_items_product_id (product_id),
    CONSTRAINT fk_order_items_order FOREIGN KEY (order_id) REFERENCES orders(id) ON DELETE CASCADE,
    CONSTRAINT fk_order_items_product FOREIGN KEY (product_id) REFERENCES products(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- -------------------------------------------------------------
-- 5. Cart Items Table (Buyer Active Cart)
-- -------------------------------------------------------------
CREATE TABLE IF NOT EXISTS cart_items (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    user_id BIGINT NOT NULL,
    product_id BIGINT NOT NULL,
    quantity INT NOT NULL CHECK (quantity > 0),
    UNIQUE KEY uq_cart_user_product (user_id, product_id),
    INDEX idx_cart_items_user_id (user_id),
    CONSTRAINT fk_cart_user FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE,
    CONSTRAINT fk_cart_product FOREIGN KEY (product_id) REFERENCES products(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- -------------------------------------------------------------
-- 6. Reviews Table (Customer Feedback & Star Ratings)
-- -------------------------------------------------------------
CREATE TABLE IF NOT EXISTS reviews (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    product_id BIGINT NOT NULL,
    user_id BIGINT NOT NULL,
    rating INT NOT NULL CHECK (rating >= 1 AND rating <= 5),
    comment TEXT,
    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
    INDEX idx_reviews_product_id (product_id),
    INDEX idx_reviews_user_id (user_id),
    CONSTRAINT fk_reviews_product FOREIGN KEY (product_id) REFERENCES products(id) ON DELETE CASCADE,
    CONSTRAINT fk_reviews_user FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- =====================================================================
-- Initial Seed Data
-- =====================================================================

-- Seed Accounts
-- Admin:   admin@ddmart.com       / Admin@123
-- Seller:  techseller@ddmart.com  / Seller@123
-- Seller:  fashionhub@ddmart.com  / Seller@123
-- Buyer:   john.buyer@ddmart.com  / Buyer@123
-- Buyer:   alice.buyer@ddmart.com / Buyer@123
INSERT IGNORE INTO users (id, name, email, password_hash, role) VALUES
(1, 'System Administrator', 'admin@ddmart.com', '$argon2id$v=19$m=65536,t=2,p=1$c29tZXNhbHRzdHJpbmc$9GZ3X3hWj74T3pLm7uT9qE4rJ8vW3xP1aZ9yK2bC4dE', 'ADMIN'),
(2, 'TechSphere Electronics', 'techseller@ddmart.com', '$argon2id$v=19$m=65536,t=2,p=1$c29tZXNhbHRzdHJpbmc$5M4kZ6yU1oN8vB3xC7aE9qW2rT4pY8sI6lK0jH2gF4d', 'SELLER'),
(3, 'Vogue & Style Hub', 'fashionhub@ddmart.com', '$argon2id$v=19$m=65536,t=2,p=1$c29tZXNhbHRzdHJpbmc$5M4kZ6yU1oN8vB3xC7aE9qW2rT4pY8sI6lK0jH2gF4d', 'SELLER'),
(4, 'John Doe', 'john.buyer@ddmart.com', '$argon2id$v=19$m=65536,t=2,p=1$c29tZXNhbHRzdHJpbmc$8Lk9J2hG4fD6sA1qW3eR5tY7uI9oP0zX2cV4bN6mM8k', 'BUYER'),
(5, 'Alice Smith', 'alice.buyer@ddmart.com', '$argon2id$v=19$m=65536,t=2,p=1$c29tZXNhbHRzdHJpbmc$8Lk9J2hG4fD6sA1qW3eR5tY7uI9oP0zX2cV4bN6mM8k', 'BUYER');

-- Products Catalog
INSERT IGNORE INTO products (id, seller_id, name, description, price_cents, stock_qty, category, image_url) VALUES
(1, 2, 'Noise-Canceling Wireless Headphones', 'Premium over-ear Bluetooth headphones with active noise cancellation, 40hr battery life, and crystal-clear audio.', 14999, 45, 'Electronics', 'https://images.unsplash.com/photo-1505740420928-5e560c06d30e?w=500&q=80'),
(2, 2, 'Mechanical Gaming Keyboard RGB', 'Customizable RGB backlit mechanical keyboard with tactile blue switches, detachable wrist rest, and N-key rollover.', 8999, 60, 'Electronics', 'https://images.unsplash.com/photo-1587829741301-dc798b83add3?w=500&q=80'),
(3, 2, 'Ultra HD 4K Monitor 27-inch', 'IPS display panel with 144Hz refresh rate, 1ms response time, HDR400, and dual HDMI 2.1 + DisplayPort.', 34999, 20, 'Electronics', 'https://images.unsplash.com/photo-1527443224154-c4a3942d3acf?w=500&q=80'),
(4, 2, 'Ergonomic Vertical Wireless Mouse', 'Ergonomic design reducing wrist strain, 4000 DPI optical sensor, quiet clicks, and rechargeable USB-C battery.', 3999, 85, 'Electronics', 'https://images.unsplash.com/photo-1615663245857-ac93bb7c39e7?w=500&q=80'),
(5, 3, 'Classic Tailored Oxford Shirt', '100% breathable organic Egyptian cotton, wrinkle-resistant finish, modern slim fit, and reinforced buttons.', 4999, 120, 'Fashion', 'https://images.unsplash.com/photo-1602810318383-e386cc2a3ccf?w=500&q=80'),
(6, 3, 'Genuine Leather Messenger Bag', 'Handcrafted full-grain vintage leather laptop messenger bag with padded compartment for up to 15.6 inch laptops.', 11999, 30, 'Fashion', 'https://images.unsplash.com/photo-1553062407-98eeb64c6a62?w=500&q=80'),
(7, 3, 'Polarized Aviator Sunglasses', 'UV400 protective polarized lenses with lightweight titanium frame, anti-glare coating, and protective hard case.', 2999, 90, 'Fashion', 'https://images.unsplash.com/photo-1511499767150-a48a237f0083?w=500&q=80'),
(8, 3, 'Minimalist Ceramic Coffee Dripper Set', 'Artisan matte ceramic pour-over cone with borosilicate glass server and 50 unbleached filters included.', 3499, 50, 'Home & Living', 'https://images.unsplash.com/photo-1514432324607-a09d9b4aefdd?w=500&q=80'),
(9, 2, 'Smart Fitness Tracker & Heart Rate Watch', 'Waterproof IP68 fitness band tracking SpO2, sleep stages, 30+ sport modes, with 14-day standby time.', 6999, 70, 'Electronics', 'https://images.unsplash.com/photo-1575311373937-040b8e1fd5b6?w=500&q=80'),
(10, 3, 'Aroma Ultrasonic Essential Oil Diffuser', '500ml capacity ultrasonic diffuser with 7 soothing ambient LED mood lights and auto-shutoff timer.', 2499, 65, 'Home & Living', 'https://images.unsplash.com/photo-1608571423902-eed4a5ad8108?w=500&q=80');

-- Orders & Order Items
INSERT IGNORE INTO orders (id, buyer_id, status, total_amount_cents, created_at) VALUES
(1, 4, 'DELIVERED', 18998, DATE_SUB(NOW(), INTERVAL 7 DAY)),
(2, 4, 'SHIPPED', 8999, DATE_SUB(NOW(), INTERVAL 2 DAY)),
(3, 5, 'CONFIRMED', 4999, DATE_SUB(NOW(), INTERVAL 1 DAY));

INSERT IGNORE INTO order_items (id, order_id, product_id, quantity, unit_price_cents) VALUES
(1, 1, 1, 1, 14999),
(2, 1, 4, 1, 3999),
(3, 2, 2, 1, 8999),
(4, 3, 5, 1, 4999);

-- Product Reviews
INSERT IGNORE INTO reviews (id, product_id, user_id, rating, comment, created_at) VALUES
(1, 1, 4, 5, 'Phenomenal sound quality and the active noise canceling blocks out all office chatter! Highly recommended.', DATE_SUB(NOW(), INTERVAL 5 DAY)),
(2, 4, 4, 4, 'Very comfortable grip, my wrist fatigue is completely gone after an 8-hour workday.', DATE_SUB(NOW(), INTERVAL 4 DAY));
