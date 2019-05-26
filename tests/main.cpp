#include "rf.hpp"

#include <gtest/gtest.h>

int main(int argc, char** argv)
{
  rf::Logger::ToLogWithFormat(rf::Logger::Info,
    "Rendering framework test suite started at %s.",
    rf::Utils::CurrentTimeDate().c_str());
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
