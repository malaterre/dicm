#include "uid.hxx"
#include <cstdlib>
#include <iostream>

#include "gtest/gtest.h"

using namespace dicm;


TEST(uid_string, size)
{
  dicm::uid_t uid;
  EXPECT_EQ(64, sizeof uid);
}

TEST(uid_bcd, size)
{
  dicm::uid_bcd_t uid;
  EXPECT_EQ(32, sizeof uid);
}

static const char * uids[] = {
  "1234",
  "1.2.3.4.5",
  "1.3.6.1.4.35045.103501438824148998807202626810206788999",
  "1.2.826.0.1.3680043.2.1143.5028470438645158236649541857909059554"
};

TEST(uid_string, print)
{
  for(const char * uid : uids)
  {
    std::cout << "value of uid: " << uid << std::endl;
    dicm::uid_t uid_str = dicm::uid_t::make_from_string(uid);
    std::cout << "value of uid_str: " << uid_str << std::endl;
  }
}

TEST(uid_bcd, print)
{
  for(const char * uid : uids)
  {
    std::cout << "value of uid: " << uid << std::endl;
    dicm::uid_bcd_t uid_bcd = dicm::uid_bcd_t::make_from_string(uid);
    std::cout << "value of uid_str: " << uid_bcd << std::endl;
  }
}

