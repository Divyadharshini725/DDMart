#include <gtest/gtest.h>
#include "util/ValidationUtil.h"
#include "util/PasswordUtil.h"

using namespace dd::ddmart::util;
using namespace dd::ddmart::exception;

TEST(ValidationTest, ValidatesEmailAddressCorrectly) {
    EXPECT_TRUE(ValidationUtil::IsValidEmail("buyer@ddmart.com"));
    EXPECT_TRUE(ValidationUtil::IsValidEmail("john.doe@sub.domain.org"));
    
    EXPECT_FALSE(ValidationUtil::IsValidEmail(""));
    EXPECT_FALSE(ValidationUtil::IsValidEmail("notanemail"));
    EXPECT_FALSE(ValidationUtil::IsValidEmail("@missinguser.com"));
    EXPECT_FALSE(ValidationUtil::IsValidEmail("missingdomain@"));

    EXPECT_NO_THROW(ValidationUtil::ValidateEmail("valid@test.com"));
    EXPECT_THROW(ValidationUtil::ValidateEmail("invalid-email"), ValidationException);
}

TEST(ValidationTest, ValidatesPasswordRules) {
    EXPECT_NO_THROW(ValidationUtil::ValidatePassword("securePass123"));
    EXPECT_THROW(ValidationUtil::ValidatePassword("123"), ValidationException);
}

TEST(ValidationTest, ValidatesRatingBounds) {
    EXPECT_NO_THROW(ValidationUtil::ValidateRating(1));
    EXPECT_NO_THROW(ValidationUtil::ValidateRating(5));
    EXPECT_THROW(ValidationUtil::ValidateRating(0), ValidationException);
    EXPECT_THROW(ValidationUtil::ValidateRating(6), ValidationException);
}

TEST(PasswordTest, HashesAndVerifiesPasswordCorrectly) {
    std::string pass = "Secret@2026";
    std::string hash = PasswordUtil::HashPassword(pass);
    EXPECT_FALSE(hash.empty());
    EXPECT_NE(pass, hash);

    EXPECT_TRUE(PasswordUtil::VerifyPassword(pass, hash));
    EXPECT_FALSE(PasswordUtil::VerifyPassword("WrongPassword", hash));
}
