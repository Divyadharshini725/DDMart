#pragma once

#include <string>
#include <algorithm>
#include <cctype>
#include <sstream>
#include "service/ChatProvider.h"

namespace dd::ddmart::service {

/**
 * @brief Canned FAQ and domain intelligence AI provider (Section 9 Rule 6 & Section 15).
 * Answers marketplace domain questions offline without external network dependencies.
 */
class MockChatProvider : public ChatProvider {
public:
    std::string GetProviderName() const override {
        return "mock";
    }

    std::string GetReply(const std::string& user_message, const std::string& /*context*/) override {
        std::string lower = ToLower(user_message);

        // 1. Return & Refund Policy
        if (lower.find("return") != std::string::npos || lower.find("refund") != std::string::npos) {
            return "DDMart offers a hassle-free 30-day return policy for all eligible items! "
                   "Products must be returned in their original packaging and unused condition. "
                   "Once the seller inspects the returned item, your refund will be processed "
                   "back to your original payment method within 3 to 5 business days.";
        }

        // 2. Shipping & Delivery
        if (lower.find("shipping") != std::string::npos || lower.find("delivery") != std::string::npos || 
            lower.find("deliver") != std::string::npos || lower.find("dispatch") != std::string::npos) {
            return "We provide fast and reliable delivery across the country: "
                   "Standard Delivery arrives in 3–5 business days, and Express Delivery arrives in 1–2 days. "
                   "You can check real-time shipment updates directly on your 'My Orders' dashboard.";
        }

        // 3. Payment Methods & Checkout
        if (lower.find("payment") != std::string::npos || lower.find("pay") != std::string::npos || 
            lower.find("card") != std::string::npos || lower.find("checkout") != std::string::npos) {
            return "DDMart supports secure instant mock checkout with Credit/Debit cards, UPI, and Net Banking. "
                   "All checkout transactions are processed atomically with inventory reservation guarantees. "
                   "Because this is a demonstration platform, no real funds are deducted during checkout.";
        }

        // 4. Seller Onboarding & Listing
        if (lower.find("seller") != std::string::npos || lower.find("sell") != std::string::npos || 
            lower.find("list product") != std::string::npos || lower.find("vendor") != std::string::npos) {
            return "Becoming a seller on DDMart is easy! Register an account with the 'SELLER' role, "
                   "navigate to your Seller Dashboard, and click 'Add Listing'. You can set product titles, "
                   "descriptions, prices (in paise/cents), categories, and stock quantities in real time.";
        }

        // 5. Order Tracking & Lifecycle Statuses
        if (lower.find("track") != std::string::npos || lower.find("status") != std::string::npos || 
            lower.find("order") != std::string::npos) {
            return "You can track your orders by visiting the 'My Orders' page in the top navigation. "
                   "Every order follows our transparent lifecycle state machine: "
                   "PENDING -> CONFIRMED -> SHIPPED -> DELIVERED. "
                   "Sellers update order progress as items are fulfilled.";
        }

        // 6. Product Reviews & Star Ratings
        if (lower.find("review") != std::string::npos || lower.find("rating") != std::string::npos || 
            lower.find("star") != std::string::npos || lower.find("feedback") != std::string::npos) {
            return "Only verified buyers who have completed an order can submit reviews and 1–5 star ratings! "
                   "This ensures 100% authentic community feedback. To review an item, open the product details "
                   "modal and share your thoughts in the review section.";
        }

        // 7. Product Catalog & Categories
        if (lower.find("product") != std::string::npos || lower.find("catalog") != std::string::npos || 
            lower.find("category") != std::string::npos || lower.find("search") != std::string::npos ||
            lower.find("item") != std::string::npos) {
            return "DDMart features top-quality products across Electronics, Fashion, Home & Living, and more! "
                   "Use the search bar at the top or click category pills to browse available inventory. "
                   "You can also sort by price, newest arrivals, or highest customer ratings.";
        }

        // 8. Account Security & Password Protection
        if (lower.find("security") != std::string::npos || lower.find("password") != std::string::npos || 
            lower.find("argon2") != std::string::npos || lower.find("safe") != std::string::npos) {
            return "Your account is hardened with military-grade Argon2id password hashing via libsodium. "
                   "Plaintext passwords are never stored or logged, and sessions are guarded against fixation attacks.";
        }

        // 9. Order Cancellation
        if (lower.find("cancel") != std::string::npos) {
            return "Orders can be cancelled free of charge while they remain in the PENDING status. "
                   "Once an order transitions to CONFIRMED or SHIPPED, please wait for delivery and "
                   "initiate a 30-day return via the orders portal.";
        }

        // 10. Customer Support / Contact
        if (lower.find("contact") != std::string::npos || lower.find("support") != std::string::npos || 
            lower.find("help") != std::string::npos || lower.find("customer care") != std::string::npos) {
            return "Our customer support team is available 24/7. You can reach out directly to support@ddmart.com "
                   "or raise an inquiry through your dashboard. We typically respond within 2 hours!";
        }

        // 11. Greetings
        if (lower.find("hello") != std::string::npos || lower.find("hi") != std::string::npos || 
            lower.find("hey") != std::string::npos || lower.find("good morning") != std::string::npos ||
            lower.find("good evening") != std::string::npos) {
            return "Hello! Welcome to DDMart. I am your AI marketplace assistant. "
                   "How can I help you today? You can ask about our catalog, order status, return policies, "
                   "or how to register as a seller!";
        }

        // Out-of-scope fallback (Section 9 Rule 2)
        return "I am the DDMart Marketplace Assistant. My expertise is strictly focused on helping you "
               "browse products, manage orders, understand shipping and return policies, or set up seller listings. "
               "How can I assist you with your shopping or store management today?";
    }

private:
    static std::string ToLower(const std::string& input) {
        std::string result = input;
        std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {
            return std::tolower(c);
        });
        return result;
    }
};

} // namespace dd::ddmart::service
