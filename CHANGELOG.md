# Changelog

All notable changes to the **DDMart** project will be documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - 2026-09-21 (Full Build Milestone Review)
- Finalized production-ready binary, multi-stage Dockerfile, systemd service, Nginx reverse proxy configuration, and full automated test suite.
- Completed security checklist: 100% parameterized SQL, Argon2id libsodium hashing, session fixation protection, and XSS auto-escaping.

## [0.5.0] - 2026-09-06
- Implemented verified product reviews and 1-5 star ratings (F8) on completed orders.
- Added strict input validation with field-level error envelope translation.

## [0.4.0] - 2026-08-30
- Implemented catalog search, category filtering, and sorting (F3).
- Added multi-step order lifecycle state machine workflow: `PENDING` -> `CONFIRMED` -> `SHIPPED` -> `DELIVERED` (O2).

## [0.3.0] - 2026-08-23
- Completed seller sales dashboard (O3) with order counts, GMV metrics, and listing CRUD (F2).
- Added admin oversight panel (F7) for platform metrics, user management, and listing moderation.

## [0.2.0] - 2026-08-16
- Implemented shopping cart management with live stock bounds checking (F4).
- Added atomic checkout transaction with mock payment confirmation (F5) and buyer/seller order views (F6).

## [0.1.0] - 2026-08-10 (MVP Checkpoint)
- Initial release featuring authentication (F1), Argon2id hashing via libsodium, session management, and SQLite database migration runner.
