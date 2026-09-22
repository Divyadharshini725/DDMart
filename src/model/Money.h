#pragma once

#include <cstdint>
#include <string>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace dd::ddmart::model {

/**
 * @brief Value object encapsulating monetary values in minor units (cents).
 *
 * FLOAT, DOUBLE, and REAL are strictly prohibited for monetary values.
 * Wraps integer minor units so no raw integer or floating-point arithmetic
 * leaks into business logic.
 */
class Money {
public:
    constexpr Money() noexcept : cents_(0) {}
    explicit constexpr Money(int64_t cents) noexcept : cents_(cents) {}

    [[nodiscard]] constexpr int64_t GetCents() const noexcept {
        return cents_;
    }

    [[nodiscard]] static Money FromDollarsAndCents(int64_t dollars, int64_t cents) {
        if (cents < 0 || cents >= 100) {
            throw std::invalid_argument("Cents must be between 0 and 99");
        }
        return Money(dollars * 100 + cents);
    }

    [[nodiscard]] std::string ToString() const {
        std::ostringstream oss;
        int64_t abs_cents = cents_ < 0 ? -cents_ : cents_;
        if (cents_ < 0) {
            oss << "-";
        }
        oss << "$" << (abs_cents / 100) << "."
            << std::setfill('0') << std::setw(2) << (abs_cents % 100);
        return oss.str();
    }

    [[nodiscard]] double ToDouble() const noexcept {
        return static_cast<double>(cents_) / 100.0;
    }

    constexpr Money operator+(const Money& other) const {
        return Money(cents_ + other.cents_);
    }

    constexpr Money operator-(const Money& other) const {
        return Money(cents_ - other.cents_);
    }

    constexpr Money operator*(int64_t factor) const {
        return Money(cents_ * factor);
    }

    Money& operator+=(const Money& other) {
        cents_ += other.cents_;
        return *this;
    }

    Money& operator-=(const Money& other) {
        cents_ -= other.cents_;
        return *this;
    }

    constexpr bool operator==(const Money& other) const noexcept { return cents_ == other.cents_; }
    constexpr bool operator!=(const Money& other) const noexcept { return cents_ != other.cents_; }
    constexpr bool operator<(const Money& other) const noexcept { return cents_ < other.cents_; }
    constexpr bool operator<=(const Money& other) const noexcept { return cents_ <= other.cents_; }
    constexpr bool operator>(const Money& other) const noexcept { return cents_ > other.cents_; }
    constexpr bool operator>=(const Money& other) const noexcept { return cents_ >= other.cents_; }

private:
    int64_t cents_{0};
};

} // namespace dd::ddmart::model
