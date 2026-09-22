#include <gtest/gtest.h>
#include "util/PasswordUtil.h"

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    dd::ddmart::util::PasswordUtil::Init();
    return RUN_ALL_TESTS();
}
