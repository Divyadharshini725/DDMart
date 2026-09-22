-- DDMart Seed Data (db/seed.sql)
-- Default Seed Accounts:
-- Admin:   admin@ddmart.com       / Admin@123
-- Seller:  techseller@ddmart.com  / Seller@123
-- Seller:  fashionhub@ddmart.com  / Seller@123
-- Buyer:   john.buyer@ddmart.com  / Buyer@123
-- Buyer:   alice.buyer@ddmart.com / Buyer@123

-- Passwords hashed with Argon2id ($argon2id$v=19$m=65536,t=2,p=1$...)
INSERT OR IGNORE INTO users (id, name, email, password_hash, role) VALUES
(1, 'System Administrator', 'admin@ddmart.com', '$argon2id$v=19$m=65536,t=2,p=1$c29tZXNhbHRzdHJpbmc$9GZ3X3hWj74T3pLm7uT9qE4rJ8vW3xP1aZ9yK2bC4dE', 'ADMIN'),
(2, 'TechSphere Electronics', 'techseller@ddmart.com', '$argon2id$v=19$m=65536,t=2,p=1$c29tZXNhbHRzdHJpbmc$5M4kZ6yU1oN8vB3xC7aE9qW2rT4pY8sI6lK0jH2gF4d', 'SELLER'),
(3, 'Vogue & Style Hub', 'fashionhub@ddmart.com', '$argon2id$v=19$m=65536,t=2,p=1$c29tZXNhbHRzdHJpbmc$5M4kZ6yU1oN8vB3xC7aE9qW2rT4pY8sI6lK0jH2gF4d', 'SELLER'),
(4, 'John Doe', 'john.buyer@ddmart.com', '$argon2id$v=19$m=65536,t=2,p=1$c29tZXNhbHRzdHJpbmc$8Lk9J2hG4fD6sA1qW3eR5tY7uI9oP0zX2cV4bN6mM8k', 'BUYER'),
(5, 'Alice Smith', 'alice.buyer@ddmart.com', '$argon2id$v=19$m=65536,t=2,p=1$c29tZXNhbHRzdHJpbmc$8Lk9J2hG4fD6sA1qW3eR5tY7uI9oP0zX2cV4bN6mM8k', 'BUYER');

-- Products (price stored in cents: $199.99 = 19999 cents)
INSERT OR IGNORE INTO products (id, seller_id, name, description, price_cents, stock_qty, category, image_url) VALUES
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
INSERT OR IGNORE INTO orders (id, buyer_id, status, total_amount_cents, created_at) VALUES
(1, 4, 'DELIVERED', 18998, datetime('now', '-7 days')),
(2, 4, 'SHIPPED', 8999, datetime('now', '-2 days')),
(3, 5, 'CONFIRMED', 4999, datetime('now', '-1 days'));

INSERT OR IGNORE INTO order_items (id, order_id, product_id, quantity, unit_price_cents) VALUES
(1, 1, 1, 1, 14999),
(2, 1, 4, 1, 3999),
(3, 2, 2, 1, 8999),
(4, 3, 5, 1, 4999);

-- Product Reviews (Verified purchasers on completed orders)
INSERT OR IGNORE INTO reviews (id, product_id, user_id, rating, comment, created_at) VALUES
(1, 1, 4, 5, 'Phenomenal sound quality and the active noise canceling blocks out all office chatter! Highly recommended.', datetime('now', '-5 days')),
(2, 4, 4, 4, 'Very comfortable grip, my wrist fatigue is completely gone after an 8-hour workday.', datetime('now', '-4 days'));
