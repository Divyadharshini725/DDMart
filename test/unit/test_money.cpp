#include <gtest/gtest.h>
#include "model/Money.h"

using namespace dd::ddmart::model;

TEST(MoneyTest, HandlesCentsConstructionAndFormatting) {
    Money m1(1999);
    EXPECT_EQ(m1.GetCents(), 1999);
    EXPECT_EQ(m1.ToString(), "₹19.99");
    EXPECT_DOUBLE_EQ(m1.ToDouble(), 19.99);

    Money zero(0);
    EXPECT_EQ(zero.ToString(), "₹0.00");

    Money single_digit_cents(5);
    EXPECT_EQ(single_digit_cents.ToString(), "₹0.05");

    Money m2 = Money::FromRupeesAndPaise(49, 99);
    EXPECT_EQ(m2.GetCents(), 4999);
    EXPECT_EQ(m2.ToString(), "₹49.99");
}

TEST(MoneyTest, ArithmeticOperations) {
    Money a(1500); // ₹15.00
    Money b(250);  // ₹2.50

    Money sum = a + b;
    EXPECT_EQ(sum.GetCents(), 1750);
    EXPECT_EQ(sum.ToString(), "₹17.50");

    Money diff = a - b;
    EXPECT_EQ(diff.GetCents(), 1250);
    EXPECT_EQ(diff.ToString(), "₹12.50");

    Money product = b * 4;
    EXPECT_EQ(product.GetCents(), 1000);
    EXPECT_EQ(product.ToString(), "₹10.00");
}

TEST(MoneyTest, ComparisonOperators) {
    Money small(100);
    Money large(200);
    Money equal(100);

    EXPECT_TRUE(small < large);
    EXPECT_TRUE(small <= large);
    EXPECT_TRUE(large > small);
    EXPECT_TRUE(large >= small);
    EXPECT_TRUE(small == equal);
    EXPECT_TRUE(small != large);
}
