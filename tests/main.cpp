#include "rf.hpp"

#include <gtest/gtest.h>

int main(int argc, char** argv)
{
  rf::Logger::ToLogWithFormat("Test suite started at %s.\n",
                              rf::Utils::CurrentTimeDate().c_str());

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

