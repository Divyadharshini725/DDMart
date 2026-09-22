# Sprint Retrospectives (RETRO.md)

## Sprint 1 (Jul 24 – Aug 2): Project Kickoff, Setup & Core Authentication
- **What Worked**: CMake + vcpkg initialization was smooth; libsodium integration provided secure Argon2id password hashing immediately.
- **What Didn't**: Initial SQLite foreign key enforcement required explicit PRAGMA configuration on startup.
- **One Change for Next Sprint**: Standardize all database connections via a central RAII database wrapper.

## Sprint 2 (Aug 3 – Aug 10): Core Flow (Browse, Cart & Checkout) - MVP Release
- **What Worked**: Layered separation between controllers and repositories allowed fast iteration of the checkout atomic transaction.
- **What Didn't**: Stock deduction race conditions during high concurrent adds to cart.
- **One Change for Next Sprint**: Utilize transactional guards and explicit stock balance checks inside `CreateOrderWithItems`.

## Sprint 3 (Aug 10 – Aug 23): Seller Portal & Admin Panel
- **What Worked**: DTO Builder patterns made serializing custom views for Seller and Admin dashboards straightforward.
- **What Didn't**: Admin moderation required fine-grained cascade deletes for related reviews and cart items.
- **One Change for Next Sprint**: Add foreign key `ON DELETE CASCADE` across all dependent tables.

## Sprint 4 (Aug 24 – Sep 6): Search, Filter, Order Lifecycle & Reviews
- **What Worked**: Client-side fetch AJAX with server-side parameterized filtering made search response times sub-10ms.
- **What Didn't**: Preventing non-buyers from posting reviews required linking orders to order_items.
- **One Change for Next Sprint**: Implement `HasBuyerPurchasedProduct` helper in Order repository to gate review submissions.

## Sprint 5 (Sep 7 – Sep 21): Security Hardening, ASan/UBSan & Deployment
- **What Worked**: Sanitizers ran clean with zero leaks due to strict RAII and `std::unique_ptr` usage; multi-stage Docker build resulted in a compact 85MB runtime container.
- **What Didn't**: Windows and Linux path separator differences in migration file discovery.
- **One Change for Next Sprint**: Use `std::filesystem::path` uniformly across all I/O runners.
