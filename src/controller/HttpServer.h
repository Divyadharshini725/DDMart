#pragma once

#include <memory>
#include <string>
#include <iostream>
#include <sstream>
#include <regex>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include "third_party/httplib.h"
#include "service/UserService.h"
#include "service/ProductService.h"
#include "service/CartService.h"
#include "service/OrderService.h"
#include "service/ReviewService.h"
#include "service/ChatService.h"
#include "dto/ChatDto.h"
#include "filter/SessionManager.h"
#include "util/JsonUtil.h"
#include "exception/ApiException.h"

namespace dd::ddmart::controller {

class HttpServer {
public:
    HttpServer(
        std::shared_ptr<service::UserService> user_service,
        std::shared_ptr<service::ProductService> product_service,
        std::shared_ptr<service::CartService> cart_service,
        std::shared_ptr<service::OrderService> order_service,
        std::shared_ptr<service::ReviewService> review_service,
        std::shared_ptr<repository::SqliteDatabase> db,
        std::shared_ptr<service::ChatService> chat_service = nullptr)
        : user_service_(std::move(user_service)),
          product_service_(std::move(product_service)),
          cart_service_(std::move(cart_service)),
          order_service_(std::move(order_service)),
          review_service_(std::move(review_service)),
          db_(std::move(db)),
          chat_service_(std::move(chat_service)) {}

    void SetupRoutes(const std::string& public_dir) {
        // Serve static assets from public/ directory
        server_.set_mount_point("/", public_dir.c_str());

        // Enable CORS and Security Headers filter
        server_.set_pre_routing_handler([this](const httplib::Request& req, httplib::Response& res) {
            ApplySecurityHeaders(res);
            if (req.method == "OPTIONS") {
                res.status = 204;
                return httplib::Server::HandlerResponse::Handled;
            }
            return httplib::Server::HandlerResponse::Unhandled;
        });

        // -------------------------------------------------------------
        // Health & Observability (Section 16)
        // -------------------------------------------------------------
        server_.Get("/api/v1/health", [this](const httplib::Request& /*req*/, httplib::Response& res) {
            bool db_up = false;
            try {
                std::lock_guard<std::mutex> lock(db_->GetMutex());
                auto stmt = db_->Prepare("SELECT 1;");
                if (stmt->Step() == SQLITE_ROW) {
                    db_up = true;
                }
            } catch (...) {
                db_up = false;
            }

            nlohmann::json health = {
                {"status", db_up ? "UP" : "DOWN"},
                {"db", db_up ? "UP" : "DOWN"},
                {"version", "1.0.0"},
                {"service", "DDMart Native C++ Backend"}
            };
            SendJsonResponse(res, 200, health);
        });

        // -------------------------------------------------------------
        // Auth Endpoints (Section 1 F1)
        // -------------------------------------------------------------
        server_.Post("/api/v1/auth/register", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                auto body = nlohmann::json::parse(req.body);
                dto::UserRegisterDto dto;
                dto.name = body.value("name", "");
                dto.email = body.value("email", "");
                dto.password = body.value("password", "");
                dto.role = body.value("role", "BUYER");

                auto user = user_service_->Register(dto);
                std::string session_id = filter::SessionManager::Instance().CreateSession(
                    user.id, user.email, user.name, user.role);

                res.set_header("Set-Cookie", "ddmart_session=" + session_id + "; Path=/; HttpOnly; SameSite=Lax");
                SendJsonResponse(res, 201, util::JsonUtil::Success(user.ToJson()));
            } catch (const exception::ApiException& e) {
                SendApiException(res, e);
            } catch (const std::exception& e) {
                SendGenericError(res, e.what());
            }
        });

        server_.Post("/api/v1/auth/login", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                auto body = nlohmann::json::parse(req.body);
                dto::UserLoginDto dto;
                dto.email = body.value("email", "");
                dto.password = body.value("password", "");

                auto user = user_service_->Login(dto);
                
                // Discard old session & issue a fresh session (Mandatory Rule 3: Session fixation protection)
                std::string old_session = ExtractSessionId(req);
                if (!old_session.empty()) {
                    filter::SessionManager::Instance().InvalidateSession(old_session);
                }

                std::string session_id = filter::SessionManager::Instance().CreateSession(
                    user.id, user.email, user.name, user.role);

                res.set_header("Set-Cookie", "ddmart_session=" + session_id + "; Path=/; HttpOnly; SameSite=Lax");
                SendJsonResponse(res, 200, util::JsonUtil::Success(user.ToJson()));
            } catch (const exception::ApiException& e) {
                SendApiException(res, e);
            } catch (const std::exception& e) {
                SendGenericError(res, e.what());
            }
        });

        server_.Post("/api/v1/auth/logout", [this](const httplib::Request& req, httplib::Response& res) {
            std::string session_id = ExtractSessionId(req);
            if (!session_id.empty()) {
                filter::SessionManager::Instance().InvalidateSession(session_id);
            }
            res.set_header("Set-Cookie", "ddmart_session=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT");
            SendJsonResponse(res, 200, util::JsonUtil::Success({{"message", "Logged out successfully"}}));
        });

        server_.Get("/api/v1/auth/me", [this](const httplib::Request& req, httplib::Response& res) {
            auto session = RequireAuth(req, res);
            if (!session) return;

            try {
                auto user = user_service_->GetUserById(session->user_id);
                SendJsonResponse(res, 200, util::JsonUtil::Success(user.ToJson()));
            } catch (const exception::ApiException& e) {
                SendApiException(res, e);
            }
        });

        // -------------------------------------------------------------
        // Products Endpoints (F2, F3)
        // -------------------------------------------------------------
        server_.Get("/api/v1/products", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                repository::ProductFilter filter;
                if (req.has_param("category")) filter.category = req.get_param_value("category");
                if (req.has_param("keyword")) filter.keyword = req.get_param_value("keyword");
                if (req.has_param("sort")) filter.sort_by = req.get_param_value("sort");
                if (req.has_param("page")) filter.page = std::stoi(req.get_param_value("page"));
                if (req.has_param("limit")) filter.limit = std::stoi(req.get_param_value("limit"));

                auto products = product_service_->SearchProducts(filter);
                int64_t total = product_service_->CountProducts(filter);

                nlohmann::json prods_arr = nlohmann::json::array();
                for (const auto& p : products) {
                    prods_arr.push_back(p.ToJson());
                }

                nlohmann::json data = {
                    {"products", prods_arr},
                    {"total", total},
                    {"page", filter.page},
                    {"limit", filter.limit}
                };
                SendJsonResponse(res, 200, util::JsonUtil::Success(data));
            } catch (const exception::ApiException& e) {
                SendApiException(res, e);
            } catch (const std::exception& e) {
                SendGenericError(res, e.what());
            }
        });

        server_.Get(R"(/api/v1/products/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
            try {
                int64_t product_id = std::stoll(req.matches[1]);
                auto product = product_service_->GetProductById(product_id);
                auto reviews = review_service_->GetProductReviews(product_id);

                nlohmann::json rev_arr = nlohmann::json::array();
                for (const auto& r : reviews) {
                    rev_arr.push_back(r.ToJson());
                }

                nlohmann::json data = product.ToJson();
                data["reviews"] = rev_arr;
                SendJsonResponse(res, 200, util::JsonUtil::Success(data));
            } catch (const exception::ApiException& e) {
                SendApiException(res, e);
            } catch (const std::exception& e) {
                SendGenericError(res, e.what());
            }
        });

        server_.Post("/api/v1/products", [this](const httplib::Request& req, httplib::Response& res) {
            auto session = RequireRole(req, res, {"SELLER", "ADMIN"});
            if (!session) return;

            try {
                auto body = nlohmann::json::parse(req.body);
                dto::ProductCreateDto dto;
                dto.name = body.value("name", "");
                dto.description = body.value("description", "");
                dto.price_cents = body.value("price_cents", 0LL);
                dto.stock_qty = body.value("stock_qty", 0);
                dto.category = body.value("category", "General");
                dto.image_url = body.value("image_url", "");

                auto product = product_service_->CreateProduct(session->user_id, dto);
                SendJsonResponse(res, 201, util::JsonUtil::Success(product.ToJson()));
            } catch (const exception::ApiException& e) {
                SendApiException(res, e);
            } catch (const std::exception& e) {
                SendGenericError(res, e.what());
            }
        });

        server_.Put(R"(/api/v1/products/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
            auto session = RequireRole(req, res, {"SELLER", "ADMIN"});
            if (!session) return;

            try {
                int64_t product_id = std::stoll(req.matches[1]);
                auto body = nlohmann::json::parse(req.body);
                dto::ProductUpdateDto dto;
                dto.name = body.value("name", "");
                dto.description = body.value("description", "");
                dto.price_cents = body.value("price_cents", 0LL);
                dto.stock_qty = body.value("stock_qty", 0);
                dto.category = body.value("category", "");
                dto.image_url = body.value("image_url", "");

                auto updated = product_service_->UpdateProduct(session->user_id, product_id, dto);
                SendJsonResponse(res, 200, util::JsonUtil::Success(updated.ToJson()));
            } catch (const exception::ApiException& e) {
                SendApiException(res, e);
            } catch (const std::exception& e) {
                SendGenericError(res, e.what());
            }
        });

        server_.Delete(R"(/api/v1/products/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
            auto session = RequireRole(req, res, {"SELLER", "ADMIN"});
            if (!session) return;

            try {
                int64_t product_id = std::stoll(req.matches[1]);
                product_service_->DeleteProduct(session->user_id, session->role, product_id);
                SendJsonResponse(res, 200, util::JsonUtil::Success({{"message", "Product deleted successfully"}}));
            } catch (const exception::ApiException& e) {
                SendApiException(res, e);
            } catch (const std::exception& e) {
                SendGenericError(res, e.what());
            }
        });

        // -------------------------------------------------------------
        // Cart Endpoints (F4)
        // -------------------------------------------------------------
        server_.Get("/api/v1/cart", [this](const httplib::Request& req, httplib::Response& res) {
            auto session = RequireRole(req, res, {"BUYER"});
            if (!session) return;

            try {
                auto cart = cart_service_->GetCart(session->user_id);
                SendJsonResponse(res, 200, util::JsonUtil::Success(cart.ToJson()));
            } catch (const exception::ApiException& e) {
                SendApiException(res, e);
            }
        });

        server_.Post("/api/v1/cart", [this](const httplib::Request& req, httplib::Response& res) {
            auto session = RequireRole(req, res, {"BUYER"});
            if (!session) return;

            try {
                auto body = nlohmann::json::parse(req.body);
                dto::CartAddDto dto;
                dto.product_id = body.value("product_id", 0LL);
                dto.quantity = body.value("quantity", 1);

                cart_service_->AddToCart(session->user_id, dto);
                auto updated_cart = cart_service_->GetCart(session->user_id);
                SendJsonResponse(res, 200, util::JsonUtil::Success(updated_cart.ToJson()));
            } catch (const exception::ApiException& e) {
                SendApiException(res, e);
            } catch (const std::exception& e) {
                SendGenericError(res, e.what());
            }
        });

        server_.Put(R"(/api/v1/cart/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
            auto session = RequireRole(req, res, {"BUYER"});
            if (!session) return;

            try {
                int64_t cart_item_id = std::stoll(req.matches[1]);
                auto body = nlohmann::json::parse(req.body);
                dto::CartUpdateDto dto;
                dto.quantity = body.value("quantity", 1);

                cart_service_->UpdateQuantity(session->user_id, cart_item_id, dto);
                auto updated_cart = cart_service_->GetCart(session->user_id);
                SendJsonResponse(res, 200, util::JsonUtil::Success(updated_cart.ToJson()));
            } catch (const exception::ApiException& e) {
                SendApiException(res, e);
            } catch (const std::exception& e) {
                SendGenericError(res, e.what());
            }
        });

        server_.Delete(R"(/api/v1/cart/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
            auto session = RequireRole(req, res, {"BUYER"});
            if (!session) return;

            try {
                int64_t cart_item_id = std::stoll(req.matches[1]);
                cart_service_->RemoveItem(session->user_id, cart_item_id);
                auto updated_cart = cart_service_->GetCart(session->user_id);
                SendJsonResponse(res, 200, util::JsonUtil::Success(updated_cart.ToJson()));
            } catch (const exception::ApiException& e) {
                SendApiException(res, e);
            }
        });

        server_.Delete("/api/v1/cart", [this](const httplib::Request& req, httplib::Response& res) {
            auto session = RequireRole(req, res, {"BUYER"});
            if (!session) return;

            cart_service_->ClearCart(session->user_id);
            SendJsonResponse(res, 200, util::JsonUtil::Success({{"message", "Cart cleared"}}));
        });

        // -------------------------------------------------------------
        // Orders & Checkout Endpoints (F5, F6, O2)
        // -------------------------------------------------------------
        server_.Post("/api/v1/orders/checkout", [this](const httplib::Request& req, httplib::Response& res) {
            auto session = RequireRole(req, res, {"BUYER"});
            if (!session) return;

            try {
                auto body = nlohmann::json::parse(req.body);
                dto::OrderCheckoutDto dto;
                dto.shipping_address = body.value("shipping_address", "");
                dto.payment_method = body.value("payment_method", "MOCK_CARD");

                auto order = order_service_->Checkout(session->user_id, dto);
                SendJsonResponse(res, 201, util::JsonUtil::Success(order.ToJson()));
            } catch (const exception::ApiException& e) {
                SendApiException(res, e);
            } catch (const std::exception& e) {
                SendGenericError(res, e.what());
            }
        });

        server_.Get("/api/v1/orders", [this](const httplib::Request& req, httplib::Response& res) {
            auto session = RequireAuth(req, res);
            if (!session) return;

            try {
                std::vector<dto::OrderResponseDto> orders;
                if (session->role == "ADMIN") {
                    orders = order_service_->GetAllOrders();
                } else if (session->role == "SELLER") {
                    orders = order_service_->GetSellerOrders(session->user_id);
                } else {
                    orders = order_service_->GetBuyerOrders(session->user_id);
                }

                nlohmann::json arr = nlohmann::json::array();
                for (const auto& o : orders) {
                    arr.push_back(o.ToJson());
                }
                SendJsonResponse(res, 200, util::JsonUtil::Success(arr));
            } catch (const exception::ApiException& e) {
                SendApiException(res, e);
            }
        });

        server_.Get(R"(/api/v1/orders/(\d+))", [this](const httplib::Request& req, httplib::Response& res) {
            auto session = RequireAuth(req, res);
            if (!session) return;

            try {
                int64_t order_id = std::stoll(req.matches[1]);
                auto order = order_service_->GetOrderById(session->user_id, session->role, order_id);
                SendJsonResponse(res, 200, util::JsonUtil::Success(order.ToJson()));
            } catch (const exception::ApiException& e) {
                SendApiException(res, e);
            }
        });

        server_.Put(R"(/api/v1/orders/(\d+)/status)", [this](const httplib::Request& req, httplib::Response& res) {
            auto session = RequireRole(req, res, {"SELLER", "ADMIN"});
            if (!session) return;

            try {
                int64_t order_id = std::stoll(req.matches[1]);
                auto body = nlohmann::json::parse(req.body);
                std::string new_status = body.value("status", "");

                auto updated = order_service_->UpdateOrderStatus(order_id, new_status);
                SendJsonResponse(res, 200, util::JsonUtil::Success(updated.ToJson()));
            } catch (const exception::ApiException& e) {
                SendApiException(res, e);
            } catch (const std::exception& e) {
                SendGenericError(res, e.what());
            }
        });

        // -------------------------------------------------------------
        // Seller Dashboard Endpoints (O3)
        // -------------------------------------------------------------
        server_.Get("/api/v1/seller/dashboard", [this](const httplib::Request& req, httplib::Response& res) {
            auto session = RequireRole(req, res, {"SELLER", "ADMIN"});
            if (!session) return;

            try {
                int64_t revenue_cents = order_service_->GetSellerRevenueCents(session->user_id);
                int64_t order_count = order_service_->GetSellerOrderCount(session->user_id);
                auto products = product_service_->GetSellerProducts(session->user_id);
                auto recent_orders = order_service_->GetSellerOrders(session->user_id);

                nlohmann::json prod_arr = nlohmann::json::array();
                for (const auto& p : products) prod_arr.push_back(p.ToJson());

                nlohmann::json ord_arr = nlohmann::json::array();
                for (const auto& o : recent_orders) ord_arr.push_back(o.ToJson());

                nlohmann::json data = {
                    {"revenue_cents", revenue_cents},
                    {"revenue_formatted", model::Money(revenue_cents).ToString()},
                    {"order_count", order_count},
                    {"product_count", static_cast<int64_t>(products.size())},
                    {"products", prod_arr},
                    {"orders", ord_arr}
                };
                SendJsonResponse(res, 200, util::JsonUtil::Success(data));
            } catch (const exception::ApiException& e) {
                SendApiException(res, e);
            }
        });

        // -------------------------------------------------------------
        // Admin Management Endpoints (F7)
        // -------------------------------------------------------------
        server_.Get("/api/v1/admin/stats", [this](const httplib::Request& req, httplib::Response& res) {
            auto session = RequireRole(req, res, {"ADMIN"});
            if (!session) return;

            try {
                int64_t gmv = order_service_->GetTotalGMVCents();
                int64_t users = user_service_->GetTotalUserCount();
                int64_t orders = order_service_->GetTotalOrderCount();
                int64_t products = product_service_->GetTotalProductCount();

                nlohmann::json stats = {
                    {"gmv_cents", gmv},
                    {"gmv_formatted", model::Money(gmv).ToString()},
                    {"user_count", users},
                    {"order_count", orders},
                    {"product_count", products}
                };
                SendJsonResponse(res, 200, util::JsonUtil::Success(stats));
            } catch (const exception::ApiException& e) {
                SendApiException(res, e);
            }
        });

        server_.Get("/api/v1/admin/users", [this](const httplib::Request& req, httplib::Response& res) {
            auto session = RequireRole(req, res, {"ADMIN"});
            if (!session) return;

            try {
                auto users = user_service_->GetAllUsers();
                nlohmann::json arr = nlohmann::json::array();
                for (const auto& u : users) arr.push_back(u.ToJson());
                SendJsonResponse(res, 200, util::JsonUtil::Success(arr));
            } catch (const exception::ApiException& e) {
                SendApiException(res, e);
            }
        });

        // -------------------------------------------------------------
        // Reviews Endpoints (F8)
        // -------------------------------------------------------------
        server_.Post(R"(/api/v1/products/(\d+)/reviews)", [this](const httplib::Request& req, httplib::Response& res) {
            auto session = RequireRole(req, res, {"BUYER"});
            if (!session) return;

            try {
                int64_t product_id = std::stoll(req.matches[1]);
                auto body = nlohmann::json::parse(req.body);
                dto::ReviewCreateDto dto;
                dto.rating = body.value("rating", 5);
                dto.comment = body.value("comment", "");

                auto review = review_service_->CreateReview(session->user_id, product_id, dto);
                SendJsonResponse(res, 201, util::JsonUtil::Success(review.ToJson()));
            } catch (const exception::ApiException& e) {
                SendApiException(res, e);
            } catch (const std::exception& e) {
                SendGenericError(res, e.what());
        });

        // -------------------------------------------------------------
        // AI Chatbot Endpoints (Phase 3 - Section 9 & 15)
        // -------------------------------------------------------------
        auto handle_chat = [this](const httplib::Request& req, httplib::Response& res) {
            try {
                if (!chat_service_) {
                    throw exception::ApiException(503, "SERVICE_UNAVAILABLE", "AI Chatbot service is not initialized");
                }

                std::string session_id = ExtractSessionId(req);
                if (session_id.empty()) {
                    if (req.has_header("X-Session-ID")) {
                        session_id = req.get_header_value("X-Session-ID");
                    } else if (!req.remote_addr.empty()) {
                        session_id = "guest_" + req.remote_addr;
                    } else {
                        session_id = "guest_default";
                    }
                }

                if (req.body.empty()) {
                    throw exception::ValidationException("Request body cannot be empty");
                }

                auto body = nlohmann::json::parse(req.body);
                std::string message = body.value("message", "");

                auto chat_resp = chat_service_->ProcessMessage(session_id, message);

                nlohmann::json data = chat_resp.ToJson();
                nlohmann::json envelope = {
                    {"success", true},
                    {"data", data},
                    {"reply", chat_resp.reply},
                    {"error", nullptr}
                };
                SendJsonResponse(res, 200, envelope);
            } catch (const exception::ApiException& e) {
                SendApiException(res, e);
            } catch (const std::exception& e) {
                SendGenericError(res, e.what());
            }
        };

        server_.Post("/api/chat", handle_chat);
        server_.Post("/api/v1/chat", handle_chat);
    }

    void Listen(const std::string& host, int port) {
        std::cout << "==================================================================" << std::endl;
        std::cout << "  DDMart Native C++ Marketplace Server is running!" << std::endl;
        std::cout << "  Access URL: http://" << host << ":" << port << "/" << std::endl;
        std::cout << "  Health API: http://" << host << ":" << port << "/api/v1/health" << std::endl;
        std::cout << "==================================================================" << std::endl;
        server_.listen(host.c_str(), port);
    }

    void Stop() {
        server_.stop();
    }

private:
    std::string ExtractSessionId(const httplib::Request& req) {
        if (req.has_header("Cookie")) {
            std::string cookies = req.get_header_value("Cookie");
            std::regex re(R"(ddmart_session=([a-zA-Z0-9_-]+))");
            std::smatch match;
            if (std::regex_search(cookies, match, re) && match.size() > 1) {
                return match[1].str();
            }
        }
        if (req.has_header("Authorization")) {
            std::string auth = req.get_header_value("Authorization");
            if (auth.rfind("Bearer ", 0) == 0) {
                return auth.substr(7);
            }
        }
        return "";
    }

    std::optional<filter::SessionData> RequireAuth(const httplib::Request& req, httplib::Response& res) {
        std::string sid = ExtractSessionId(req);
        if (sid.empty()) {
            SendApiException(res, exception::UnauthorizedException("Authentication required: please log in"));
            return std::nullopt;
        }

        auto session = filter::SessionManager::Instance().GetSession(sid);
        if (!session.has_value()) {
            SendApiException(res, exception::UnauthorizedException("Session expired or invalid: please log in again"));
            return std::nullopt;
        }
        return session;
    }

    std::optional<filter::SessionData> RequireRole(
        const httplib::Request& req, httplib::Response& res, const std::vector<std::string>& allowed_roles) {
        auto session = RequireAuth(req, res);
        if (!session) return std::nullopt;

        bool allowed = false;
        for (const auto& r : allowed_roles) {
            if (session->role == r) {
                allowed = true;
                break;
            }
        }

        if (!allowed) {
            SendApiException(res, exception::ForbiddenException("Forbidden: insufficient privileges for " + session->role));
            return std::nullopt;
        }
        return session;
    }

    static void ApplySecurityHeaders(httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization, Cookie");
        res.set_header("Access-Control-Allow-Credentials", "true");
        res.set_header("X-Content-Type-Options", "nosniff");
        res.set_header("X-Frame-Options", "DENY");
        res.set_header("X-XSS-Protection", "1; mode=block");
    }

    static void SendJsonResponse(httplib::Response& res, int status, const nlohmann::json& body) {
        res.status = status;
        res.set_content(body.dump(), "application/json; charset=utf-8");
    }

    static void SendApiException(httplib::Response& res, const exception::ApiException& e) {
        res.status = e.GetStatusCode();
        res.set_content(e.ToJson().dump(), "application/json; charset=utf-8");
    }

    static void SendGenericError(httplib::Response& res, const std::string& message) {
        res.status = 500;
        nlohmann::json err = util::JsonUtil::Error("SERVER_ERROR", message);
        res.set_content(err.dump(), "application/json; charset=utf-8");
    }

    httplib::Server server_;
    std::shared_ptr<service::UserService> user_service_;
    std::shared_ptr<service::ProductService> product_service_;
    std::shared_ptr<service::CartService> cart_service_;
    std::shared_ptr<service::OrderService> order_service_;
    std::shared_ptr<service::ReviewService> review_service_;
    std::shared_ptr<service::ChatService> chat_service_;
    std::shared_ptr<repository::SqliteDatabase> db_;
};

} // namespace dd::ddmart::controller
