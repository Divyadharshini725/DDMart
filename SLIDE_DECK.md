# DDMart — Capstone Final Review Presentation Slide Deck
**Native C++20 Marketplace Backend Specification**  
**Milestone**: Final Review (October 10, 2026)  
**Builder**: Solo Project (`dd::ddmart`)

---

## Slide 1: Title & Project Overview
* **Title**: DDMart — High-Performance Native C++ Marketplace Backend
* **Specification Target**: Equivalent to Java Servlets/JDBC Capstone, engineered in modern C++20
* **Key Technologies**: C++20, CMake, SQLite 3 / PostgreSQL, libsodium (Argon2id), spdlog, nlohmann::json
* **Deliverable Milestone**: Final Capstone Review (October 10, 2026)
* **Status**: 100% Complete across Mandatory (F1–F8) & Optional (O2, O3, O4) Milestones

---

## Slide 2: Architectural Principles & Separation of Concerns
* **3-Tier Decomposition**:
  1. **Filter & Controller Layer**: Thin HTTP orchestration, CORS & security header enforcement, session extraction, and standardized API error envelopes (`/api/v1/*`).
  2. **Service Layer**: Pure business logic with zero direct database queries; validates input bounds, calculates metrics, and orchestrates workflows.
  3. **Repository Layer**: 100% parameterized SQL prepared statements; completely abstracts database dialects behind abstract interfaces.
* **Database Support**: Embedded SQLite 3 (WAL mode) for development/testing, PostgreSQL 15+ for enterprise production.

---

## Slide 3: Mandatory Feature Implementation (F1 through F8)
* **F1 Authentication & Roles**: Buyer, Seller, and Seeded Admin; Argon2id password hashing; session fixation defense.
* **F2 Seller Listings**: Complete CRUD lifecycle for marketplace inventory (titles, descriptions, pricing, stock, categories).
* **F3 Buyer Catalog**: Combined keyword search, category filtering, sorting (price, date, rating), and pagination.
* **F4 Shopping Cart**: Real-time stock reservation checks, atomic quantity adjustments, and running totals.
* **F5 Checkout & Mock Payment**: Atomic transactional deduction of inventory with mock card payment verification.
* **F6 Order History & Tracking**: Segmented views: buyers track orders; sellers manage fulfillment queues.
* **F7 Admin Oversight**: Comprehensive platform oversight: GMV statistics, user directories, order auditing, and product moderation.
* **F8 Verified Reviews**: 1–5 star ratings and reviews restricted strictly to verified buyers who purchased the item.

---

## Slide 4: Advanced Workflow Extensions (O2, O3)
* **O2 Order Status State Machine**:
  - Deterministic lifecycle: `PENDING` &rarr; `CONFIRMED` &rarr; `SHIPPED` &rarr; `DELIVERED` (or `CANCELLED`).
  - Sellers and Admins transition orders through stages as goods are dispatched.
* **O3 Seller Sales Analytics**:
  - Real-time revenue calculations and order volume counters.
  - Safe financial arithmetic using the encapsulated `Money` minor-unit value object (paise/cents).

---

## Slide 5: Phase 3 AI Chatbot Architecture (O4)
* **Component Architecture**:
  - **Frontend**: Floating interactive chat widget with quick FAQ suggestion chips and typing indicators.
  - **Endpoints**: `POST /api/chat` and `POST /api/v1/chat` returning standardized JSON envelopes.
  - **ChatService**: Enforces guardrails, manages caching, and coordinates providers.
* **Strategy Pattern**: Abstract `ChatProvider` interface supports:
  - `MockChatProvider`: Offline domain FAQ answering (10+ marketplace topics).
  - `GeminiChatProvider`: Cloud LLM integration via Google Gemini API.
* **Factory Pattern**: `ChatProviderFactory` selects provider via `AI_CHATBOT_PROVIDER`.

---

## Slide 6: AI Guardrails & Reliability Hardening
* **Per-Session Rate Limiting**:
  - Hard limit of **10 messages per minute** per session ID, tracked via a mutex-guarded in-memory sliding window. Rejections return HTTP 429.
* **Deduplication In-Memory Cache**:
  - Repeated identical queries within a session return instant cached replies (`cached = true`).
* **Fixed Server-Side Prompt Template**:
  - Strict guardrails keep responses restricted to DDMart products, shipping, returns, and seller onboarding.
* **Degraded Fallback Resilience**:
  - Outbound calls feature explicit 5-second timeouts. Network failures fall back to static domain FAQ answers without 500 errors.

---

## Slide 7: Design Patterns Showcase (Section 10)
1. **Repository Pattern**: `IUserRepository`, `IProductRepository`, `ICartRepository`, etc.
2. **Front Controller Pattern**: Centralized router, session extraction, and security filters in `HttpServer`.
3. **Singleton Pattern**: Central thread-safe session store via `SessionManager::Instance()`.
4. **Factory Pattern**: `ChatProviderFactory` for dynamic AI provider instantiation.
5. **Strategy Pattern**: Polymorphic `ChatProvider` algorithm families.
6. **Builder Pattern**: Fluent DTO projection methods preventing credential leaks.
7. **Dependency Injection**: Constructor-injected repository interfaces across all service classes.

---

## Slide 8: Security & Reliability Engineering
* **100% Parameterized SQL**: Zero string concatenation; all queries use prepared statements.
* **Argon2id Hashing**: Industry-standard password hashing via libsodium `crypto_pwhash_str`.
* **Session Fixation Defense**: Pre-auth sessions invalidated upon login; fresh cryptographic tokens issued.
* **Zero Information Leakage**: Universal exception translator maps errors to fixed JSON schemas.
* **Memory Safety**: Strict RAII; zero raw owning pointers; zero memory leaks on AddressSanitizer (ASan) & UBSan.

---

## Slide 9: Automated Regression Test Suite
* **Unit Testing**:
  - `test_money.cpp`: Validates integer arithmetic, boundary values, and string formatting.
  - `test_validation.cpp`: Tests email patterns, password constraints, and review bounds.
  - `test_chat.cpp`: Validates FAQ answers, 10 msg/min rate limiting, in-memory caching, and degraded fallbacks.
* **Repository Testing**:
  - `test_sqlite_repositories.cpp`: Full ACID CRUD testing on `:memory:` SQLite database.
* **Continuous Integration**:
  - GitHub Actions pipeline runs automated build, sanitizers, and `ctest` on every push.

---

## Slide 10: Deployment Ready & Live Demo Walkthrough
* **Deployment Options**:
  - Standalone compiled binary (`ddmart`).
  - Systemd background service unit (`ddmart.service`).
  - Nginx reverse proxy with TLS termination.
  - Multi-stage Docker container (~85MB runtime image).
* **Live Demo Highlights**:
  1. Buyer registration, product search, and responsive filtering.
  2. Cart management and atomic mock checkout.
  3. Seller dashboard, inventory updates, and order fulfillment.
  4. Verified customer review submission.
  5. Interactive AI Chatbot FAQ assistance and rate-limit verification.
