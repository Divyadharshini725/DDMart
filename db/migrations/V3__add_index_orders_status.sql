-- Migration V3: Add Index on Orders Status
CREATE INDEX IF NOT EXISTS idx_orders_status ON orders(status);
