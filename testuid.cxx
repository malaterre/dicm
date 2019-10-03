#include "vr.hxx"
#include <cstdlib>
#include <iostream>

#include "gtest/gtest.h"

using namespace dicm;


TEST(uid_string, size)
{
  uid_t uid;
  EXPECT_EQ(64, sizeof uid);
}
