# DDMart — Rehearsed Demonstration Script & Evaluation Playbook
**Checkpoint Window: July 27 – October 10, 2026**  
**Milestone**: Final Review (October 10, 2026)  
**Deliverable**: 2–3 Minute Screen-Capture Demonstration & Live Walkthrough  
**Target URL**: `http://localhost:8080/` (or deployed public URL)

---

## 1. Pre-Demo Setup & Verification

### Step 1: Start the Backend Application
```bash
# In the DDMart repository root:
./build/ddmart
# Expected output:
# >>> Initializing DDMart Native C++ Marketplace...
# >>> Applying schema migrations...
# >>> Seeding default accounts and catalog...
# ==================================================================
#   DDMart Native C++ Marketplace Server is running!
#   Access URL: http://0.0.0.0:8080/
#   Health API: http://0.0.0.0:8080/api/v1/health
# ==================================================================
```

### Step 2: Verify System Health Probe
Run a quick health check in the terminal or browser:
```bash
curl -s http://localhost:8080/api/v1/health
# Returns:
# {"db":"UP","service":"DDMart Native C++ Backend","status":"UP","version":"1.2.0"}
```

---

## 2. Seed Credentials Reference Table

| Role | Email Address | Password | Primary Capabilities Demonstrated |
|---|---|---|---|
| **Admin** | `admin@ddmart.com` | `Admin@123` | Platform oversight, GMV stats, user list, product moderation |
| **Seller** | `techseller@ddmart.com` | `Seller@123` | Manage electronics listings, fulfillment queue, sales dashboard |
| **Seller** | `fashionhub@ddmart.com` | `Seller@123` | Manage apparel listings, fulfillment queue, sales dashboard |
| **Buyer** | `john.buyer@ddmart.com` | `Buyer@123` | Product browsing, cart management, mock checkout, verified reviews |
| **Buyer** | `alice.buyer@ddmart.com` | `Buyer@123` | Product browsing, cart management, mock checkout, verified reviews |

---

## 3. Timed 2–3 Minute Video Demonstration Walkthrough

### [0:00 – 0:30] Phase 1: Storefront Browsing, Category Filtering & AI Assistant Intro
1. **Open Storefront**: Navigate to `http://localhost:8080/`.
   - Point out clean responsive design and the brand badge: `DDMart Native C++20`.
2. **Category Filtering & Search (F3)**:
   - Click the **Electronics** pill &rarr; Catalog dynamically filters to smartphones, headphones, and laptops.
   - Type `keyboard` in the search bar &rarr; Instant sub-10ms debounced search results.
   - Change sort dropdown to **Price: Low to High** &rarr; Products reorder instantaneously.
3. **Trigger AI Chat Widget (O4)**:
   - Click the floating circular robot icon in the bottom-right corner.
   - Click the quick action chip: **"↩️ Return Policy"**.
   - Show instantaneous AI reply explaining DDMart's 30-day return policy and refund process.

---

### [0:30 – 1:00] Phase 2: Shopping Cart & Atomic Checkout
1. **Product Detail Modal**:
   - Click on the **Sony WH-1000XM5 Wireless Headphones**.
   - Show product details modal, price, seller info, and verified customer reviews.
   - Click **"🛒 Add to Cart"** &rarr; Notice instant toast notification and cart badge count updating to `1`.
2. **Cart Management (F4)**:
   - Click **🛒 Cart** in top navigation (`/cart.html`).
   - Increase item quantity from `1` to `2` &rarr; Subtotal and Grand Total update using `Money` value object.
3. **Atomic Mock Checkout (F5)**:
   - Click **"Proceed to Checkout"**.
   - Enter shipping address: `221B Baker Street, London`.
   - Select payment method: `Instant Mock Card`.
   - Click **"Confirm & Place Order"** &rarr; Atomic transaction deducts inventory, clears cart, and redirects to `/orders.html`.

---

### [1:00 – 1:30] Phase 3: Order Tracking & Verified Purchaser Reviews
1. **Buyer Order Tracking (F6, O2)**:
   - On `/orders.html`, show the freshly placed order.
   - Status is currently **`PENDING`**.
2. **Reviewing Completed Orders (F8)**:
   - Show past delivered orders from seed data.
   - Click **"⭐ Leave Review"** on a delivered product.
   - Rate **5 Stars** with comment: *"Exceptional sound quality and rapid delivery!"*.
   - Click submit &rarr; Success toast appears; non-purchased items do not permit reviews.

---

### [1:30 – 2:00] Phase 4: Seller Dashboard & Order Fulfillment Workflow
1. **Log in as Seller**:
   - Log out, then log in as `techseller@ddmart.com` / `Seller@123`.
2. **Sales Analytics Dashboard (O3)**:
   - Navigate to `/seller.html`.
   - Review live metrics: **Total Revenue**, **Orders Fulfilled**, and active product listings.
3. **Order Fulfillment State Machine (O2)**:
   - Locate the incoming order placed by John Buyer in Phase 2.
   - Click status transition button: **"Mark as Confirmed"** &rarr; Transitions to `CONFIRMED`.
   - Click **"Mark as Shipped"** &rarr; Transitions to `SHIPPED`.
4. **Create New Listing (F2)**:
   - Click **"+ Add New Product"**.
   - Enter name: *"Mechanical Gaming Keyboard"*, category: *Electronics*, price: *₹79.99*, stock: *25*.
   - Click Save &rarr; Item appears in seller catalog immediately.

---

### [2:00 – 2:30] Phase 5: Admin Panel Oversight & Moderation
1. **Log in as Admin**:
   - Log out, then log in as `admin@ddmart.com` / `Admin@123`.
2. **Platform Statistics (F7)**:
   - Navigate to `/admin.html`.
   - Review total platform **GMV (Gross Merchandise Value)**, registered user count, total orders, and listings.
3. **Listing Moderation**:
   - Locate product listings under Platform Catalog Moderation.
   - Moderate/remove test listing &rarr; Cascades safely without database constraint violations.

---

### [2:30 – 3:00] Phase 6: AI Chatbot Deep Dive & Rate-Limit Verification
1. **AI Chatbot Guardrails (Section 9 & 15)**:
   - Open Chatbot Widget.
   - Ask: *"How long does shipping take?"* &rarr; Returns detailed shipping timeframe (Standard 3–5 days, Express 1–2 days).
   - Ask identical question again: *"How long does shipping take?"* &rarr; Returns immediate response with `cached: true` (0ms response time).
2. **Rate Limit Verification**:
   - Send 10 rapid queries in terminal or via widget:
     ```bash
     for i in {1..11}; do
       curl -s -X POST http://localhost:8080/api/chat \
         -H "Content-Type: application/json" \
         -d '{"message":"ping"}'
     done
     ```
   - 11th request returns HTTP 429:
     ```json
     {
       "success": false,
       "data": null,
       "error": {
         "code": "RATE_LIMIT_EXCEEDED",
         "message": "Too many chat requests. Rate limit is 10 messages per minute."
       }
     }
     ```
3. **Summary & Wrap-Up**:
   - Point out zero compiler warnings, zero sanitizer violations, clean memory lifecycle, and complete documentation.

---

## 4. Evaluation Q&A Cheat Sheet

* **Q: Where is all SQL located?**
  - **A**: Exclusively in `src/repository/` behind abstract interfaces. Zero SQL statements leak into controllers or services. Every single statement is parameterized with `sqlite3_bind_*`.
* **Q: How are passwords hashed?**
  - **A**: Argon2id via libsodium `crypto_pwhash_str`. No plaintext passwords exist in the database or logs.
* **Q: How does the AI Chatbot handle network failure?**
  - **A**: `ChatService` wraps outbound calls in explicit 5-second timeouts and catches any exceptions, returning a static degraded FAQ response without failing the HTTP request.
* **Q: How are currency values managed?**
  - **A**: Minor units (cents/paise) stored as `int64_t` wrapped in the `Money` value object (`src/model/Money.h`). Floating-point arithmetic is prohibited for currency fields.
