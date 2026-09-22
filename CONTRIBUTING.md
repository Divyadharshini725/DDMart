# Contributing to DDMart

Thank you for contributing to **DDMart**, a modern native C++20 marketplace backend application!

## Prerequisites

- **C++ Compiler**: GCC 12+, Clang 15+, or MSVC 2022+ with C++20 standard support.
- **CMake**: Version 3.25 or later.
- **Package Manager**: vcpkg or system packages (libsodium, sqlite3, spdlog, nlohmann-json, gtest).

## Quickstart Setup & Local Build

### 1. Clone the Repository
```bash
git clone https://github.com/ddmart/ddmart.git
cd ddmart
```

### 2. Install Dependencies via vcpkg
```bash
# If using vcpkg
vcpkg install
```
Or on Debian/Ubuntu:
```bash
sudo apt update && sudo apt install -y build-essential cmake ninja-build \
    libsqlite3-dev libsodium-dev libspdlog-dev nlohmann-json3-dev libgtest-dev
```

### 3. Configure and Build with CMake
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### 4. Run the Automated Test Suite
```bash
ctest --test-dir build --output-on-failure
```

### 5. Launch the DDMart Backend
```bash
./build/ddmart
```
Once launched, open your web browser at:
- **Web Storefront**: [http://localhost:8080/](http://localhost:8080/)
- **Health Check**: [http://localhost:8080/api/v1/health](http://localhost:8080/api/v1/health)

## Code Standards
- **Style**: Follow Google C++ Style Guide (`PascalCase` types, `snake_case_` member variables).
- **Security**: 100% parameterized SQL queries (`sqlite3_bind_*`).
- **Memory Safety**: Strict RAII; no raw owning pointers (`std::unique_ptr` / `std::shared_ptr` only).
- **Sanitizers**: Code must pass AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan) clean.
